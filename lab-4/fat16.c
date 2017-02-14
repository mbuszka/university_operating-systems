/*
 * This file is based on example source code 'hello_ll.c'
 *
 * FUSE: Filesystem in Userspace
 * Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
 *
 * This program can be distributed under the terms of the GNU GPL.
 */

#define FUSE_USE_VERSION 30

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse_lowlevel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "fat16.h"
#include "blkio.h"

#define DEBUG 1
#define TRACE 1

#define UNUSED __attribute__((unused))

#define panic()                                                 \
  __extension__({                                               \
    fprintf(stderr, "%s: %s\n", __func__, strerror(errno));     \
    exit(EXIT_FAILURE);                                         \
  })

typedef struct fuse_file_info fuse_file_info_t;

typedef struct {
  char *p;
  size_t size;
} dirbuf_t;

static struct {
  char*        fs_store_path;
  int          fs_store;
  bootsector_t fs_bootsector;
  uid_t fs_uid;
  gid_t fs_gid;
} fsinfo;

#define BPB(field) (((struct bpb50 *) (&fsinfo.fs_bootsector.bsBPB))->field)
#define RESERVED_START  0
#define RESERVED_SIZE   BPB(bpbResSectors)
#define NFATS           BPB(bpbFATs)
#define FAT_SIZE        BPB(bpbFATsecs)
#define CLUSTER_SIZE    BPB(bpbSecPerClust)
#define ROOT_DIR        (RESERVED_SIZE + NFATS * FAT_SIZE)
#define ROOT_DIR_ENTS   BPB(bpbRootDirEnts)
#define ROOT_DIR_SIZE   ((ROOT_DIR_ENTS * sizeof(direntry_t)) % BLKSIZE == 0 ? \
                         (ROOT_DIR_ENTS * sizeof(direntry_t) / BLKSIZE)        \
                        :(ROOT_DIR_ENTS * sizeof(direntry_t) / BLKSIZE) + 1)
#define DATA_START      (ROOT_DIR + ROOT_DIR_SIZE)
#define NBLOCKS         (BPB(bpbSectors) == 0 ? BPB(bpbHugeSectors) \
                                              : BPB(bpbSectors))
#define DATA_SIZE       (NBLOCKS - (RESERVED_SIZE +                 \
                                    NFATS*FAT_SIZE + ROOT_DIR_SIZE))
#define ENTRIES_IN_CLUSTER (CLUSTER_SIZE * BLKSIZE / sizeof(direntry_t))
#define CONSTRUCT_INODE(_blck, _offset) ((_blck << 32) + _offset)
#define DIRENTRY_BLOCK(  inode) (inode >> 32)
#define DIRENTRY_OFFSET( inode) (inode & 0x00000000ffffffff)
#define C_FOR_EACH(_cluster, _fat) \
  for ( ; _cluster < 0xFFF8; _cluster = _fat[_cluster] )
#define min(x, y) ((x) < (y) ? (x) : (y))

static void unpack_name(const char *src, char *dst);
static void pack_name(const char *src, char *dst);
static u_int16_t *load_fat();

static void fat16_init(void                  *userdata UNUSED
                     , struct fuse_conn_info *conn     UNUSED) {
  void *buf;

  if (TRACE) fprintf(stderr, "%s\n", __func__);

  if (posix_memalign(&buf, BLKSIZE, BLKSIZE) != 0)
    panic();

  fsinfo.fs_store = blk_open(fsinfo.fs_store_path);
  blk_read(fsinfo.fs_store, buf, 0, 1);
  memcpy(&fsinfo.fs_bootsector, buf, BLKSIZE);
  fsinfo.fs_uid = getuid();
  fsinfo.fs_gid = getgid();

  if (DEBUG) {
    fprintf(stderr,
           "Initialized FAT-16 filesystem.\n"
           "Boot sector info:\n"
           "OEM Name : %s\n"
           "Signature : %x%x\n"
           "Root dir addr : %x\n"
           "Root dir size : %lu\n"
          , (char*) &fsinfo.fs_bootsector.bsOemName
          , fsinfo.fs_bootsector.bsBootSectSig0
          , fsinfo.fs_bootsector.bsBootSectSig1
          , ROOT_DIR * BLKSIZE
          , ROOT_DIR_SIZE);
  }

  free(buf);
}

static size_t count_clusters(direntry_t entry, u_int16_t *fat) {
  size_t    n           = 0;
  u_int16_t cluster     = entry.deStartCluster;
  int       should_free = 0;

  if (fat == NULL) {
    fat = load_fat();
    should_free = 1;
  }

  C_FOR_EACH(cluster, fat) {
    n ++;
  }

  if (should_free)
    free(fat);
  return n;
}

static direntry_t *load_directory_cluster(direntry_t *buf, u_int16_t cluster) {
  size_t block;

  if (buf == NULL) {
    if (posix_memalign((void **) &buf, BLKSIZE, BLKSIZE * CLUSTER_SIZE) != 0)
      panic();
  }

  block = DATA_START + CLUSTER_SIZE * (cluster - 2);
  blk_read(fsinfo.fs_store, buf, block, CLUSTER_SIZE);

  return buf;
}

static direntry_t *load_root_contents() {
  void *root;

  if (posix_memalign(&root, BLKSIZE, BLKSIZE * ROOT_DIR_SIZE) != 0)
    panic();

  blk_read(fsinfo.fs_store, root, ROOT_DIR, ROOT_DIR_SIZE);
  return (direntry_t *) root;
}

static time_t convert_time(u_int16_t time, u_int16_t date) {
  struct tm t;
  memset(&t, 0, sizeof(struct tm));
  t.tm_sec   = ((time & DT_2SECONDS_MASK)  >> DT_2SECONDS_SHIFT) * 2;
  t.tm_min   = ( time & DT_MINUTES_MASK)   >> DT_MINUTES_SHIFT;
  t.tm_hour  = ( time & DT_HOURS_MASK)     >> DT_HOURS_SHIFT;
  t.tm_mday  = ( date & DD_DAY_MASK)       >> DD_DAY_SHIFT;
  t.tm_mon   = ((date & DD_MONTH_MASK)     >> DD_MONTH_SHIFT) - 1;
  t.tm_year  = ((date & DD_YEAR_MASK)      >> DD_YEAR_SHIFT) + 80;
  t.tm_isdst = -1;

  if (TRACE) fprintf(stderr, "%s : %d:%d:%d %d-%d-%d\n"
                           , __func__
                           , t.tm_hour
                           , t.tm_min
                           , t.tm_sec
                           , t.tm_mday
                           , t.tm_mon
                           , t.tm_year);
  return mktime(&t);
}

static direntry_t load_direntry(fuse_ino_t ino) {
  void      *buf;
  direntry_t entry;
  size_t     blk = DIRENTRY_BLOCK(ino);
  size_t     off = DIRENTRY_OFFSET(ino);

  if ( TRACE ) fprintf(stderr, "%s : loaded entry at %lx\n"
                             , __func__
                             , blk * BLKSIZE + off * sizeof(direntry_t));

  if (posix_memalign(&buf, BLKSIZE, BLKSIZE) != 0)
    panic();

  blk_read(fsinfo.fs_store, buf, blk, 1);
  memcpy(&entry, buf + (sizeof(direntry_t)) * off, sizeof(direntry_t));
  free(buf);
  return entry;
}

static size_t count_root_subdirectories() {
  size_t      cnt = 0;
  direntry_t *dir = load_root_contents();

  for (size_t i=0; i<ROOT_DIR_ENTS; i++) {
    if (dir[i].deAttributes & ATTR_DIRECTORY &&
        dir[i].deName[0] != '.')
      cnt++;
  }

  free(dir);
  return cnt;
}

static size_t count_subdirectories(direntry_t handle, u_int16_t *fat) {
  size_t      cnt         = 0;
  direntry_t *dir         = NULL;
  u_int16_t   cluster     = handle.deStartCluster;
  int         should_free = 0;

  if (fat == NULL) {
    fat = load_fat();
    should_free = 1;
  }

  C_FOR_EACH(cluster, fat) {
    dir = load_directory_cluster(dir, cluster);
    for (size_t i=0; i<ENTRIES_IN_CLUSTER; i++) {
      if (dir[i].deName[0] == SLOT_EMPTY) break;
      if (dir[i].deAttributes & ATTR_DIRECTORY)
        cnt++;
    }
  }

  if (should_free)
    free(fat);
  return cnt;
}

static void stat_root(struct stat *stbuf) {
  stbuf->st_mode = S_IFDIR | 0755;
  stbuf->st_uid = fsinfo.fs_uid;
  stbuf->st_gid = fsinfo.fs_gid;
  stbuf->st_size = ROOT_DIR_SIZE * BLKSIZE;
  stbuf->st_blocks = ROOT_DIR_SIZE;
  stbuf->st_nlink = 2 + count_root_subdirectories();
}

static void stat_sys_dir(struct stat *stbuf, direntry_t entry) {
  u_int16_t *fat = load_fat();
  stbuf->st_mode = S_IFDIR | 0550;
  stbuf->st_uid = 0;
  stbuf->st_gid = 0;
  stbuf->st_blocks = CLUSTER_SIZE * count_clusters(entry, fat);
  stbuf->st_size = stbuf->st_blocks * BLKSIZE;
  stbuf->st_nlink = 2 + count_subdirectories(entry, fat);
  if (!(entry.deAttributes & ATTR_READONLY))
    stbuf->st_mode |= 0220;
  free(fat);
}

static void stat_usr_dir(struct stat *stbuf, direntry_t entry) {
  u_int16_t *fat = load_fat();
  stbuf->st_mode = S_IFDIR | 0555;
  stbuf->st_uid = fsinfo.fs_uid;
  stbuf->st_gid = fsinfo.fs_gid;
  stbuf->st_blocks = CLUSTER_SIZE * count_clusters(entry, fat);
  stbuf->st_size = stbuf->st_blocks * BLKSIZE;
  stbuf->st_nlink = 1 + count_subdirectories(entry, fat);
  if (!(entry.deAttributes & ATTR_READONLY))
    stbuf->st_mode |= 0220;
  free(fat);
}

static void stat_sys_reg(struct stat *stbuf, direntry_t entry) {
  stbuf->st_mode = S_IFREG | 0544;
  stbuf->st_uid = 0;
  stbuf->st_gid = 0;
  stbuf->st_size   = entry.deFileSize;
  stbuf->st_blocks = entry.deFileSize / BLKSIZE +
    (entry.deFileSize % BLKSIZE ? 1 : 0);
  stbuf->st_nlink = 1;
  if (!(entry.deAttributes & ATTR_READONLY))
    stbuf->st_mode |= 0220;
}

static void stat_usr_reg(struct stat *stbuf, direntry_t entry) {
  stbuf->st_mode = S_IFREG | 0544;
  stbuf->st_uid = fsinfo.fs_uid;
  stbuf->st_gid = fsinfo.fs_gid;
  stbuf->st_size   = entry.deFileSize;
  stbuf->st_blocks = entry.deFileSize / BLKSIZE +
    (entry.deFileSize % BLKSIZE ? 1 : 0);
  stbuf->st_nlink = 1;
  if (!(entry.deAttributes & ATTR_READONLY))
    stbuf->st_mode |= 0220;
}

static int fat16_stat(fuse_ino_t ino, struct stat *stbuf) {
  stbuf->st_ino = ino;
  stbuf->st_blksize = BLKSIZE;

  if (TRACE) fprintf(stderr, "%s\n", __func__);

  if (ino == 1) {
    stat_root(stbuf);
  } else {
    direntry_t entry = load_direntry(ino);
    if (entry.deAttributes & ATTR_SYSTEM) {
      if (entry.deAttributes & ATTR_DIRECTORY)
        stat_sys_dir(stbuf, entry);
      else
        stat_sys_reg(stbuf, entry);
    } else {
      if (entry.deAttributes & ATTR_DIRECTORY)
        stat_usr_dir(stbuf, entry);
      else
        stat_usr_reg(stbuf, entry);
    }
    stbuf->st_atime = convert_time(0, entry.deADate);
    stbuf->st_mtime = convert_time( entry.deMTime
                                  , entry.deMDate);
    stbuf->st_ctime = stbuf->st_mtime;
  }
  return 0;
}

static void fat16_getattr(fuse_req_t req, fuse_ino_t ino,
                          fuse_file_info_t *fi UNUSED) {
  struct stat stbuf;

  if (TRACE) fprintf(stderr, "%s\n", __func__);

  memset(&stbuf, 0, sizeof(stbuf));
  if (fat16_stat(ino, &stbuf) == -1) {
    fuse_reply_err(req, ENOENT);
  } else {
    fuse_reply_attr(req, &stbuf, 1.0);
  }
}

static void fat16_lookup(fuse_req_t req, fuse_ino_t parent_ino, const char *name) {
  struct fuse_entry_param e;
  direntry_t  parent;
  char        packed_name[11];
  direntry_t *directory = NULL;
  size_t      ino = 0;

  if (TRACE) fprintf(stderr, "%s : name=%s, parent=%lu\n"
                           , __func__, name, parent_ino);

  pack_name(name, packed_name);

  if (parent_ino == 1) {
    directory = load_root_contents();
    for (int i=0; i<ROOT_DIR_ENTS; i++) {
      if (strncmp(packed_name, (char*) directory[i].deName, 11) == 0 ) {
        ino = CONSTRUCT_INODE((u_int64_t) (ROOT_DIR + i * 32 / BLKSIZE)
                             ,(u_int64_t) (i % (BLKSIZE / 32)));
        break;
      }
    }
  } else {
    parent = load_direntry(parent_ino);
    u_int64_t  block;
    u_int16_t *fat     = load_fat();
    u_int16_t  cluster = parent.deStartCluster;

    C_FOR_EACH(cluster, fat) {
      block     = DATA_START + CLUSTER_SIZE * (cluster - 2);
      directory = load_directory_cluster(directory, cluster);
      unsigned int i = 0;
      while ( directory[i].deName[0] != SLOT_EMPTY && i < ENTRIES_IN_CLUSTER ) {
        if ( strncmp(packed_name, (char*) directory[i].deName, 11) == 0 ) {
          ino = CONSTRUCT_INODE( (u_int64_t) (block + i * 32 / BLKSIZE)
                               , (u_int64_t) (i % (BLKSIZE / 32)));
          break;
        }
        i++;
      }
      if ( i < ENTRIES_IN_CLUSTER ) break;
    }
  }
  if (ino == 0) {
    fuse_reply_err(req, ENOENT);
    return;
  }
  memset(&e, 0, sizeof(e));
  e.ino = ino;
  e.attr_timeout = 1.0;
  e.entry_timeout = 1.0;
  fat16_stat(e.ino, &e.attr);
  fuse_reply_entry(req, &e);
}

static void dirbuf_add(fuse_req_t req, dirbuf_t *b, const char *name,
                       fuse_ino_t ino) {
  struct stat stbuf;
  size_t oldsize = b->size;
  b->size += fuse_add_direntry(req, NULL, 0, name, NULL, 0);
  b->p = (char *)realloc(b->p, b->size);
  memset(&stbuf, 0, sizeof(stbuf));
  stbuf.st_ino = ino;
  fuse_add_direntry(req, b->p + oldsize, b->size - oldsize, name, &stbuf,
                    b->size);
}

static int reply_buf_limited(fuse_req_t req, const char *buf, size_t bufsize,
                             off_t off, size_t maxsize) {
  if (off < (ssize_t)bufsize) {
    return fuse_reply_buf(req, buf + off, min(bufsize - off, maxsize));
  } else {
    return fuse_reply_buf(req, NULL, 0);
  }
}

static void fat16_opendir(fuse_req_t req, fuse_ino_t ino UNUSED,
                          fuse_file_info_t *fi) {
  if (TRACE) fprintf(stderr, "%s\n", __func__);
  fuse_reply_open(req, fi);
}

static void populate_buffer(dirbuf_t *b, fuse_req_t req, direntry_t *dir
                                       , size_t block, size_t len) {
  char   entry_name[13];
  size_t ino;

  for (size_t i=0 ; i < len ; i++ ) {
    if ( dir[i].deAttributes & ATTR_VOLUME ) continue;
    if ( dir[i].deName[0] == SLOT_EMPTY ) break;
    if ( dir[i].deName[0] == SLOT_DELETED ) continue;
    unpack_name((char *) dir[i].deName, entry_name);
    ino = CONSTRUCT_INODE((u_int64_t) (block + i * 32 / BLKSIZE)
                         ,(u_int64_t) (i % (BLKSIZE / 32)));
    dirbuf_add(req, b, entry_name, ino);
  }
}

static void fat16_readdir(fuse_req_t req, fuse_ino_t ino, size_t size,
                          off_t off, fuse_file_info_t *fi UNUSED) {
  if (TRACE) fprintf(stderr, "%s\n", __func__);

  dirbuf_t    b;
  direntry_t *dir_contents = NULL;
  memset(&b, 0, sizeof(b));

  if (ino == 1) {
    dir_contents = load_root_contents();
    populate_buffer(&b, req, dir_contents, ROOT_DIR, ROOT_DIR_ENTS);
  } else {
    u_int64_t  block;
    direntry_t dir     = load_direntry(ino);
    u_int16_t *fat     = load_fat();
    u_int16_t  cluster = dir.deStartCluster;

    if (!(dir.deAttributes & ATTR_DIRECTORY)) {
      fuse_reply_err(req, ENOTDIR);
      return;
    }

    C_FOR_EACH(cluster, fat) {
      dir_contents = load_directory_cluster(dir_contents, cluster);
      block        = DATA_START + CLUSTER_SIZE * (cluster - 2);
      populate_buffer(&b, req, dir_contents, block, ENTRIES_IN_CLUSTER);
    }
  }

  reply_buf_limited(req, b.p, b.size, off, size);
  free(dir_contents);
  free(b.p);
}

static void fat16_releasedir(fuse_req_t req, fuse_ino_t ino UNUSED,
                             fuse_file_info_t *fi UNUSED) {
  if (TRACE) fprintf(stderr, "%s\n", __func__);

  fuse_reply_err(req, 0);
}

static void fat16_open(fuse_req_t req
                    , fuse_ino_t ino UNUSED
                    , fuse_file_info_t *fi) {
  if (TRACE) fprintf(stderr, "%s\n", __func__);
  fuse_reply_open(req, fi);
}

static void fat16_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
                       fuse_file_info_t *fi UNUSED) {
  if (TRACE) fprintf(stderr, "%s\n", __func__);

  direntry_t file = load_direntry(ino);

  if ( file.deFileSize < off ) {
    fuse_reply_buf(req, NULL, 0);
    return;
  }

  char *buf;
  size = min(size, file.deFileSize - off);
  u_int16_t  cluster = file.deStartCluster;
  u_int16_t *fat     = load_fat();
  u_int16_t  start   = off / (CLUSTER_SIZE * BLKSIZE);
  size_t     cl_cnt  = size / (CLUSTER_SIZE * BLKSIZE) +
                              (size % (CLUSTER_SIZE * BLKSIZE) ? 1 : 0);
  size_t     i       = 0;

  C_FOR_EACH(cluster, fat) {
    if (i >= start) break;
    i++;
  }
  i = 0;
  if (cluster >= 0xFFF8) {
    fuse_reply_buf(req, NULL, 0);
    free(fat);
    return;
  }

  if (posix_memalign((void **) &buf, BLKSIZE, cl_cnt * CLUSTER_SIZE * BLKSIZE) != 0 )
    panic();

  C_FOR_EACH(cluster, fat) {
    if ( i >= cl_cnt ) break;
    if (TRACE) fprintf(stderr, "cluster addr : %lx\n",  (DATA_START + CLUSTER_SIZE * (cluster - 2)) * BLKSIZE);
    blk_read(fsinfo.fs_store, buf + (i * CLUSTER_SIZE * BLKSIZE)
                            , DATA_START + CLUSTER_SIZE * (cluster - 2), CLUSTER_SIZE);
    i++;
  }

  fuse_reply_buf(req, buf + (off % (CLUSTER_SIZE * BLKSIZE)), size);
  free(fat);
}

static void fat16_release(fuse_req_t req, fuse_ino_t ino UNUSED,
                          fuse_file_info_t *fi UNUSED) {
  if (TRACE) fprintf(stderr, "%s\n", __func__);
  fuse_reply_err(req, 0);
}

static void fat16_statfs(fuse_req_t req, fuse_ino_t ino UNUSED) {
  struct statvfs statfs;

  if (TRACE) fprintf(stderr, "%s\n", __func__);

  memset(&statfs, 0, sizeof(statfs));
  statfs.f_bsize = CLUSTER_SIZE * BLKSIZE;
  statfs.f_frsize = CLUSTER_SIZE * BLKSIZE;
  statfs.f_blocks = DATA_SIZE / CLUSTER_SIZE;
  u_int16_t *fat = load_fat();
  if (TRACE) fprintf(stderr, "fat_size : %d, clusters : %lu\n"
                           , FAT_SIZE * BLKSIZE / 16
                           , DATA_SIZE);
  for (unsigned int i=2; i<statfs.f_blocks + 2; i++) {
    if (fat[i] == 0) statfs.f_bfree ++;
  }
  if (TRACE) fprintf(stderr, "det\n");
  statfs.f_bavail = statfs.f_bfree;
  statfs.f_namemax = 8;
  fuse_reply_statfs(req, &statfs);
  free(fat);
}

static struct fuse_lowlevel_ops fat16_oper = {
  .init   = fat16_init,
  .lookup = fat16_lookup,
  .getattr = fat16_getattr,
  .opendir = fat16_opendir,
  .readdir = fat16_readdir,
  .releasedir = fat16_releasedir,
  .open = fat16_open,
  .read = fat16_read,
  .release = fat16_release,
  .statfs = fat16_statfs
};

int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "Usage : %s <fs-image RAW> <mountpoint> [opts]", argv[0]);
    return 1;
  }

  char *fargs[argc-1];

  fsinfo.fs_store_path = argv[1];
  fargs[0] = strdup(argv[0]);
  for (int i=2; i<argc; i++) {
    fargs[i-1] = strdup(argv[i]);
  }
  argc--;

  struct fuse_args args = FUSE_ARGS_INIT(argc, fargs);
  struct fuse_chan *ch;
  char *mountpoint;
  int err = -1;

  if (fuse_parse_cmdline(&args, &mountpoint, NULL, NULL) != -1 &&
      (ch = fuse_mount(mountpoint, &args)) != NULL) {
    struct fuse_session *se;

    se = fuse_lowlevel_new(&args, &fat16_oper, sizeof(fat16_oper), NULL);
    if (se != NULL) {
      if (fuse_set_signal_handlers(se) != -1) {
        fuse_session_add_chan(se, ch);
        err = fuse_session_loop(se);
        fuse_remove_signal_handlers(se);
        fuse_session_remove_chan(ch);
      }
      fuse_session_destroy(se);
    }
    fuse_unmount(mountpoint, ch);
  }
  fuse_opt_free_args(&args);

  for (int i=0; i<argc; i++) {
    free(fargs[i]);
  }

  return err ? 1 : 0;
}

static u_int16_t *load_fat() {
  u_int16_t *fat;
  if ( posix_memalign((void **) &fat, BLKSIZE, BLKSIZE * FAT_SIZE) != 0 )
    panic();

  blk_read( fsinfo.fs_store, fat, RESERVED_SIZE, FAT_SIZE );
  return fat;
}

// change name from internal format, to name.ext,
// note that dst can be 13 characters long (including terminating null)
static void unpack_name(const char *src, char *dst) {

  if (TRACE) fprintf(stderr, "%s\n", __func__);

  dst[0] = ((u_int8_t) src[0]) == SLOT_E5 ? 0xe5 : src[0];
  int j = 1;
  for (int i = 1; i<8; i++) {
    if (src[i] == ' ' ) break;
    dst[j] = src[i];
    j++;
  }
  if (src[8] == ' ' ) {
    dst[j] = '\0';
    return;
  } else {
    dst[j] = '.';
    j++;
  }
  for (int i = 8; i<11; i++) {
    if (src[i] == ' ' ) break;
    dst[j] = src[i];
    j++;
  }
  dst[j] = '\0';
}

static void pack_name(const char *src, char *dst) {
  dst[0] = ((u_int8_t) src[0]) == 0xe5 ? SLOT_E5 : src[0];
  int j = 1;
  int i = 1;

  while ( src[i] != '.' && src[i] != '\0' ) {
    dst[j] = src[i];
    i++; j++;
  }

  for ( ; j < 8; j++ ) {
    dst[j] = ' ';
  }

  if ( src[i] == '\0' ) {
    for ( ; j < 11; j++ )
      dst[j] = ' ';
  } else {
    i++;
    while ( src[i] != '\0' ) {
      dst[j] = src[i];
      i++; j++;
    }
  }
}
