/*-
 * Written by Paul Popelka (paulp@uts.amdahl.com)
 *
 * You can do anything you want with this software, just don't say you wrote
 * it, and don't remove this notice.
 *
 * This software is provided "as is".
 *
 * The author supplies this software to be publicly redistributed on the
 * understanding that the author is not responsible for the correct
 * functioning of this software in any circumstances and is not liable for
 * any damages caused by this software.
 *
 * October 1992
 */
#ifndef _FAT16_H_
#define _FAT16_H_

/*
 * Format of a boot sector.  This is the first sector on a DOS floppy disk
 * or the first sector of a partition on a hard disk.  But, it is not the
 * first sector of a partitioned hard disk.
 */
typedef struct bootsector33 {
  u_int8_t bsJump[3];     /* jump inst E9xxxx or EBxx90 */
  int8_t bsOemName[8];    /* OEM name and version */
  int8_t bsBPB[19];       /* BIOS parameter block */
  int8_t bsDriveNumber;   /* drive number (0x80) */
  int8_t bsBootCode[479]; /* pad so struct is 512b */
  u_int8_t bsBootSectSig0;
  u_int8_t bsBootSectSig1;
#define BOOTSIG0 0x55
#define BOOTSIG1 0xaa
} bootsector_t;

struct extboot {
  int8_t exDriveNumber;   /* drive number (0x80) */
  int8_t exReserved1;     /* reserved */
  int8_t exBootSignature; /* ext. boot signature (0x29) */
#define EXBOOTSIG 0x29
  int8_t exVolumeID[4];     /* volume ID number */
  int8_t exVolumeLabel[11]; /* volume label */
  int8_t exFileSysType[8];  /* fs type (FAT12 or FAT16) */
};

struct bootsector50 {
  u_int8_t bsJump[3];     /* jump inst E9xxxx or EBxx90 */
  int8_t bsOemName[8];    /* OEM name and version */
  int8_t bsBPB[25];       /* BIOS parameter block */
  int8_t bsExt[26];       /* Bootsector Extension */
  int8_t bsBootCode[448]; /* pad so structure is 512b */
  u_int8_t bsBootSectSig0;
  u_int8_t bsBootSectSig1;
#define BOOTSIG0 0x55
#define BOOTSIG1 0xaa
};

struct bootsector710 {
  u_int8_t bsJump[3];     /* jump inst E9xxxx or EBxx90 */
  int8_t bsOEMName[8];    /* OEM name and version */
  int8_t bsBPB[53];       /* BIOS parameter block */
  int8_t bsExt[26];       /* Bootsector Extension */
  int8_t bsBootCode[420]; /* pad so structure is 512b */
  u_int8_t bsBootSectSig0;
  u_int8_t bsBootSectSig1;
#define BOOTSIG0 0x55
#define BOOTSIG1 0xaa
};

union bootsector {
  struct bootsector33 bs33;
  struct bootsector50 bs50;
  struct bootsector710 bs710;
};

/*
 * BIOS Parameter Block (BPB) for DOS 3.3
 */
struct bpb33 {
  u_int16_t bpbBytesPerSec; /* bytes per sector */
  u_int8_t  bpbSecPerClust; /* sectors per cluster */
  u_int16_t bpbResSectors;  /* number of reserved sectors */
  u_int8_t  bpbFATs;        /* number of FATs */
  u_int16_t bpbRootDirEnts; /* number of root directory entries */
  u_int16_t bpbSectors;     /* total number of sectors */
  u_int8_t  bpbMedia;       /* media descriptor */
  u_int16_t bpbFATsecs;     /* number of sectors per FAT */
  u_int16_t bpbSecPerTrack; /* sectors per track */
  u_int16_t bpbHeads;       /* number of heads */
  u_int16_t bpbHiddenSecs;  /* number of hidden sectors */
} __attribute__((packed));

/*
 * BPB for DOS 5.0 The difference is bpbHiddenSecs is a short for DOS 3.3,
 * and bpbHugeSectors is not in the 3.3 bpb.
 */
struct bpb50 {
  u_int16_t bpbBytesPerSec; /* bytes per sector */
  u_int8_t bpbSecPerClust;  /* sectors per cluster */
  u_int16_t bpbResSectors;  /* number of reserved sectors */
  u_int8_t bpbFATs;         /* number of FATs */
  u_int16_t bpbRootDirEnts; /* number of root directory entries */
  u_int16_t bpbSectors;     /* total number of sectors */
  u_int8_t bpbMedia;        /* media descriptor */
  u_int16_t bpbFATsecs;     /* number of sectors per FAT */
  u_int16_t bpbSecPerTrack; /* sectors per track */
  u_int16_t bpbHeads;       /* number of heads */
  u_int32_t bpbHiddenSecs;  /* # of hidden sectors */
  u_int32_t bpbHugeSectors; /* # of sectors if bpbSectors == 0 */
} __attribute__((packed));

/*
 * BPB for DOS 7.10 (FAT32).  This one has a few extensions to bpb50.
 */
struct bpb710 {
  u_int16_t bpbBytesPerSec; /* bytes per sector */
  u_int8_t bpbSecPerClust;  /* sectors per cluster */
  u_int16_t bpbResSectors;  /* number of reserved sectors */
  u_int8_t bpbFATs;         /* number of FATs */
  u_int16_t bpbRootDirEnts; /* number of root directory entries */
  u_int16_t bpbSectors;     /* total number of sectors */
  u_int8_t bpbMedia;        /* media descriptor */
  u_int16_t bpbFATsecs;     /* number of sectors per FAT */
  u_int16_t bpbSecPerTrack; /* sectors per track */
  u_int16_t bpbHeads;       /* number of heads */
  u_int32_t bpbHiddenSecs;  /* # of hidden sectors */
  u_int32_t bpbHugeSectors; /* # of sectors if bpbSectors == 0 */
  u_int32_t bpbBigFATsecs;  /* like bpbFATsecs for FAT32 */
  u_int16_t bpbExtFlags;    /* extended flags: */
#define FATNUM 0xf          /* mask for numbering active FAT */
#define FATMIRROR 0x80      /* FAT is mirrored (like it always was) */
  u_int16_t bpbFSVers;      /* filesystem version */
#define FSVERS 0            /* currently only 0 is understood */
  u_int32_t bpbRootClust;   /* start cluster for root directory */
  u_int16_t bpbFSInfo;      /* filesystem info structure sector */
  u_int16_t bpbBackup;      /* backup boot sector */
  u_int8_t bpbReserved[12]; /* reserved for future expansion */
} __attribute__((packed));

/*
 * FAT32 FSInfo block.
 */
struct fsinfo {
  u_int8_t fsisig1[4];
  u_int8_t fsifill1[480];
  u_int8_t fsisig2[4];
  u_int8_t fsinfree[4];
  u_int8_t fsinxtfree[4];
  u_int8_t fsifill2[12];
  u_int8_t fsisig3[4];
};

/*
 * Structure of a dos directory entry.
 */
struct direntry {
  u_int8_t deName[11];        /* filename, blank filled */
#define SLOT_EMPTY 0x00       /* slot has never been used */
#define SLOT_E5 0x05          /* the real value is 0xe5 */
#define SLOT_DELETED 0xe5     /* file in this slot deleted */
  u_int8_t deAttributes;      /* file attributes */
#define ATTR_NORMAL 0x00      /* normal file */
#define ATTR_READONLY 0x01    /* file is readonly */
#define ATTR_HIDDEN 0x02      /* file is hidden */
#define ATTR_SYSTEM 0x04      /* file is a system file */
#define ATTR_VOLUME 0x08      /* entry is a volume label */
#define ATTR_DIRECTORY 0x10   /* entry is a directory name */
#define ATTR_ARCHIVE 0x20     /* file is new or modified */
  u_int8_t deLowerCase;       /* NT VFAT lower case flags */
#define LCASE_BASE 0x08       /* filename base in lower case */
#define LCASE_EXT 0x10        /* filename extension in lower case */
  u_int8_t deCHundredth;      /* hundredth of seconds in CTime */
  u_int16_t deCTime;          /* create time */
  u_int16_t deCDate;          /* create date */
  u_int16_t deADate;          /* access date */
  u_int16_t deHighClust;      /* high bytes of cluster number */
  u_int16_t deMTime;          /* last update time */
  u_int16_t deMDate;          /* last update date */
  u_int16_t deStartCluster;   /* starting cluster of file */
  u_int32_t deFileSize;       /* size of file in bytes */
} __attribute__((packed));
typedef struct direntry direntry_t;

/*
 * Structure of a Win95 long name directory entry
 */
struct winentry {
  u_int8_t weCnt;
#define WIN_LAST 0x40
#define WIN_CNT 0x3f
  u_int8_t wePart1[10];
  u_int8_t weAttributes;
#define ATTR_WIN95 0x0f
  u_int8_t weReserved1;
  u_int8_t weChksum;
  u_int8_t wePart2[12];
  u_int16_t weReserved2;
  u_int8_t wePart3[4];
};
#define WIN_CHARS 13 /* Number of chars per winentry */

/*
 * Maximum number of winentries for a filename.
 */
#define WIN_MAXSUBENTRIES 20

/*
 * Maximum filename length in Win95
 * Note: Must be < sizeof(dirent.d_name)
 */
#define WIN_MAXLEN 255

/*
 * This is the format of the contents of the deTime field in the direntry
 * structure.
 * We don't use bitfields because we don't know how compilers for
 * arbitrary machines will lay them out.
 */
#define DT_2SECONDS_MASK 0x1F /* seconds divided by 2 */
#define DT_2SECONDS_SHIFT 0
#define DT_MINUTES_MASK 0x7E0 /* minutes */
#define DT_MINUTES_SHIFT 5
#define DT_HOURS_MASK 0xF800 /* hours */
#define DT_HOURS_SHIFT 11

/*
 * This is the format of the contents of the deDate field in the direntry
 * structure.
 */
#define DD_DAY_MASK 0x1F /* day of month */
#define DD_DAY_SHIFT 0
#define DD_MONTH_MASK 0x1E0 /* month */
#define DD_MONTH_SHIFT 5
#define DD_YEAR_MASK 0xFE00 /* year - 1980 */
#define DD_YEAR_SHIFT 9

#endif /* !_FAT16_H_ */
