#ifndef ARENA_H
#define ARENA_H

#include <sys/types.h>      // size_t, ssize_t
#include <stdint.h>         // uint64_t
#include <bsd/sys/queue.h>  // lists
#include <unistd.h>         // getpagesize()
#include <stddef.h>         // offsetof()
#include <pthread.h>        // pthread_mutex_t

/* STRUCTURES */
typedef struct mem_block {

  /* all blocks list entry */
  LIST_ENTRY(mem_block) mb_list;

  /* mb_size > 0 => block is free, mb_size < 0 => block is allocated */
  ssize_t               mb_size;
  uint64_t              magic;
  union {

    /* free blocks list entry, valid iff block is free */
    LIST_ENTRY(mem_block) mb_free_list;

    /* user data pointer, valid iff block is allocated */
    uint64_t              mb_data[0];
  };
} mem_block_t;

typedef struct mem_arena {

  /* all arenas list entry */
  LIST_ENTRY(mem_arena)  ma_list;

  /* list of all free blocks in this arena */
  LIST_HEAD(, mem_block) ma_freeblks;

  /* list of all blocks */
  LIST_HEAD(, mem_block) ma_blocks;

  /* arena size minus sizeof(mem_arena_t) */
  size_t                 ma_size;
} mem_arena_t;

typedef struct mem_ctrl {
  LIST_HEAD(, mem_arena) mc_arenas;
  void                  *mc_next_addr;
  pthread_mutex_t        mutex;
} mem_ctrl_t;

/* CONSTANTS */
#define PAGESIZE ((size_t) getpagesize())
#define BIG_ARENA_TRESHOLD (3 * PAGESIZE)
#define DEFAULT_ARENA_SIZE (4 * PAGESIZE - sizeof(mem_arena_t))
#define DEFAULT_ALIGNMENT  (sizeof(void*))
#define EXCLUSIVE_ARENA_TRESHOLD 1024

/* MEMORY ARENA OPERATIONS */
mem_arena_t *spawn_arena(size_t size);
int          remove_arena(mem_arena_t *arena);
void         dump_arena(mem_arena_t *arena, mem_block_t *highlight);
void         dump_all();

/* MEMORY BLOCK OPERATIONS */
void        *allocate_block(mem_arena_t *arena, size_t size, size_t alignment);
mem_block_t *merge_block(mem_arena_t *arena, mem_block_t* blk);
mem_block_t *shift_block(mem_arena_t *arena, mem_block_t* blk, size_t diff);
mem_block_t *split_block(mem_arena_t *arena, mem_block_t* src, void *split);
void         insert_free_block(mem_arena_t *arena, mem_block_t *blk);

/* UTILITY FUNCTIONS */
size_t align_ptr(void **ptr, size_t alignment);
size_t sabs(ssize_t x);
#define MEM_BLOCK_HDR_S offsetof(mem_block_t, mb_data)
#define is_power_of_two(number) (((number) != 0) && !((number) & ((number) - 1)))
#define sgn(number) ((number) >= 0 ? 1 : -1)

/* From stack overflow */
#define __containerof(ptr, type, member) ({                     \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})


#endif
