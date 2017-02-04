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

static struct {
  char*        fs_store_path;
  int          fs_store;
  bootsector_t fs_bootsector;
} fsinfo;

#define BPB(field) (((struct bpb33 *) (&fsinfo.fs_bootsector.bsBPB))->field)
#define RESERVED_START  0
#define RESERVED_SIZE   BPB(bpbResSectors)
#define NFATS           BPB(bpbFATs)
#define SECTORS_PER_FAT BPB(bpbFATsecs)
#define CLUSTER_SIZE    BPB(bpbSecPerClust)
#define ROOT_DIR        (RESERVED_SIZE + NFATS * SECTORS_PER_FAT)
#define ROOT_DIR_ENTS   BPB(bpbRootDirEnts)
#define ROOT_DIR_SIZE   ((ROOT_DIR_ENTS * 32) % BLKSIZE == 0 ? \
                         (ROOT_DIR_ENTS * 32)                  \
                        :(ROOT_DIR_ENTS * 32) + 1)

static const char *fat16_str  = "Hello World!\n";
static const char *fat16_name = "hello";

static void fat16_init( void                  *userdata UNUSED
                      , struct fuse_conn_info *conn     UNUSED ) {
  void *buf;

  if (TRACE) fprintf(stderr, "%s\n", __func__);

  if ( posix_memalign(&buf, BLKSIZE, BLKSIZE) != 0 )
    panic();

  fsinfo.fs_store = blk_open(fsinfo.fs_store_path);
  blk_read(fsinfo.fs_store, buf, 0, 1);
  memcpy(&fsinfo.fs_bootsector, buf, BLKSIZE);
  printf("Initialized FAT-16 filesystem.\n"
         "Boot sector info:\n"
         "OEM Name : %s\n"
         "Signature : %x%x\n"
        , (char*) &fsinfo.fs_bootsector.bsOemName
        , fsinfo.fs_bootsector.bsBootSectSig0
        , fsinfo.fs_bootsector.bsBootSectSig1);
  free(buf);
}

#define DIRENTRY_CLUSTER(inode) (inode >> 16)
#define DIRENTRY_OFFSET( inode) (inode & 0x00ff)

static int fat16_stat(fuse_ino_t ino, struct stat *stbuf) {
  stbuf->st_ino = ino;

  if (TRACE) fprintf(stderr, "%s\n", __func__);

  if (ino == 1) {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
  } else {
    size_t cluster = DIRENTRY_CLUSTER(ino);
    size_t offset  = DIRENTRY_OFFSET(ino);
    dentry_t *directory;
    if ( posix_memalign(&directory, BLKSIZE, BLKSIZE) != 0 ) {
      panic();
    }

    blk_read(fsinfo.fs_store, directory, cluster * CLUSTER_SIZE, 1);
    if ( directory[offset].deAttributes & ATTR_DIRECTORY ) {
      stbuf->st_mode = S_IFDIR | 0755;

    } else {
      stbuf->st_mode = S_IFREG | 0444;
    }
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

static void fat16_lookup(fuse_req_t req, fuse_ino_t parent, const char *name) {
  struct fuse_entry_param e;

  if (TRACE) fprintf(stderr, "%s\n", __func__);

  if (parent != 1 || strcmp(name, fat16_name) != 0) {
    fuse_reply_err(req, ENOENT);
  } else {
    memset(&e, 0, sizeof(e));
    e.ino = 2;
    e.attr_timeout = 1.0;
    e.entry_timeout = 1.0;
    fat16_stat(e.ino, &e.attr);

    fuse_reply_entry(req, &e);
  }
}

typedef struct {
  char *p;
  size_t size;
} dirbuf_t;

// change name from internal format, to name.ext,
// note that dst can be 13 characters long (including terminating null)
static void unpack_name(char *src, char *dst) {
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

static void pack_name(char *src, char *dst) {
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

  if ( src[i] == '\0' )
  i++;
  while ( src[i] != '\0' ) {

  }
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

#define min(x, y) ((x) < (y) ? (x) : (y))

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

static void fat16_readdir(fuse_req_t req, fuse_ino_t ino, size_t size,
                          off_t off, fuse_file_info_t *fi UNUSED) {

  if (TRACE) fprintf(stderr, "%s\n", __func__);

  if (ino != 1) {
    fuse_reply_err(req, ENOTDIR);
  } else {
    dirbuf_t  b;
    dentry_t *root_dir;
    char entry_name[13];

    memset(&b, 0, sizeof(b));
    if ( posix_memalign(&root_dir, BLKSIZE, ROOT_DIR_SIZE * BLKSIZE) != 0 ) {
      fprintf(stderr, "Couldn't read root directory\n");
      panic();
    }

    blk_read(fsinfo.fs_store, root_dir, ROOT_DIR, ROOT_DIR_SIZE);

    for (int i = 0; i < ROOT_DIR_ENTS; i ++) {
      if ( root_dir[i].deName[0] == SLOT_EMPTY ) continue;
      if ( root_dir[i].deName[0] == SLOT_DELETED ) continue;
      if ( root_dir[i].deName[0] == SLOT_E5 ) {
        entry_name[0] = 0xe5;
      } else {
        entry_name[0] = root_dir[i].deName[0];
      }

      // entr


      size_t ino = (ROOT_DIR + i * 32 / BLKSIZE) << 16;
      ino += i % (BLKSIZE / 32);
      dirbuf_add(req, &b, entry_name, ino);
    }
    reply_buf_limited(req, b.p, b.size, off, size);
    free(root_dir);
    free(b.p);
  }
}

static void fat16_releasedir(fuse_req_t req, fuse_ino_t ino UNUSED,
                             fuse_file_info_t *fi UNUSED) {
  if (TRACE) fprintf(stderr, "%s\n", __func__);

  fuse_reply_err(req, ENOENT);
}

static void fat16_open(fuse_req_t req, fuse_ino_t ino, fuse_file_info_t *fi) {
  if (TRACE) fprintf(stderr, "%s\n", __func__);

  // if (ino != 2)
  //   fuse_reply_err(req, EISDIR);
  // else if ((fi->flags & 3) != O_RDONLY)
  //   fuse_reply_err(req, EACCES);
  // else
  fuse_reply_open(req, fi);
}

static void fat16_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
                       fuse_file_info_t *fi UNUSED) {
  if (TRACE) fprintf(stderr, "%s\n", __func__);
  assert(ino == 2);
  reply_buf_limited(req, fat16_str, strlen(fat16_str), off, size);
}

static void fat16_release(fuse_req_t req, fuse_ino_t ino UNUSED,
                          fuse_file_info_t *fi UNUSED) {
  if (TRACE) fprintf(stderr, "%s\n", __func__);
  fuse_reply_err(req, ENOENT);
}

static void fat16_statfs(fuse_req_t req, fuse_ino_t ino UNUSED) {
  struct statvfs statfs;

  if (TRACE) fprintf(stderr, "%s\n", __func__);

  memset(&statfs, 0, sizeof(statfs));
  fuse_reply_statfs(req, &statfs);
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
