#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "arena.h"

extern mem_ctrl_t mem_ctrl;

/* MEMORY ARENA OPERATIONS */

mem_arena_t *spawn_arena(size_t size) {
  int          ps;
  size_t       space_needed;
  mem_arena_t *new_arena;
  mem_block_t *blk;

  ps = getpagesize();
  space_needed = size + sizeof(mem_arena_t);
  space_needed = space_needed % ps
                 ? (space_needed / ps + 1) * ps
                 : space_needed;

  new_arena = mmap( NULL // mem_ctrl.mc_next_addr
                  , space_needed
                  , PROT_READ | PROT_WRITE
                  , MAP_PRIVATE | MAP_ANONYMOUS
                  , -1, 0);
  if (new_arena == MAP_FAILED) {
    printf("Error : %s\n", strerror(errno));
  }

  new_arena->ma_size = space_needed - sizeof(mem_arena_t);
  mem_ctrl.mc_next_addr = (void *) (((size_t) new_arena) + space_needed);

  blk = (mem_block_t *) (new_arena + 1);
  blk->mb_size =  new_arena->ma_size - MEM_BLOCK_HDR_S;
  blk->magic = 0xDEADC0DE;
  LIST_INSERT_HEAD(&new_arena->ma_blocks, blk, mb_list);
  LIST_INSERT_HEAD(&new_arena->ma_freeblks, blk, mb_free_list);

  LIST_INSERT_HEAD(&mem_ctrl.mc_arenas, new_arena, ma_list);
  return new_arena;
}

int remove_arena(mem_arena_t *arena) {
  LIST_REMOVE(arena, ma_list);
  munmap(arena, arena->ma_size + sizeof(mem_arena_t));
  return 0;
}

void dump_arena(mem_arena_t *arena, mem_block_t *highlight) {
  mem_block_t *el;
  int i=0;
  printf("Dumping arena\n\tbeginning : %lx\n\tsize : %lu\n"
        , (size_t) arena
        , arena->ma_size
        );
  LIST_FOREACH(el, &arena->ma_blocks, mb_list) {
    assert(el->magic == 0xDEADC0DE);
    printf("%s [ %d ] header : %lx, data : %lx, size : %ld\n"
          , el == highlight ? " >>\t" : "\t"
          , i
          , (size_t) el
          , (size_t) el->mb_data
          , el->mb_size
          );
    i++;
  }
}

void dump_all() {
  mem_arena_t *ar;
  LIST_FOREACH(ar, &mem_ctrl.mc_arenas, ma_list) {
    if (ar == NULL) return;
    dump_arena(ar, NULL);
  }
}

/* MEMORY BLOCK OPERATIONS */

void *allocate_block(mem_arena_t *arena, size_t size, size_t alignment) {
  mem_block_t *blk;
  void *data_ptr = NULL;
  size_t diff = 0;

  blk = LIST_FIRST(&arena->ma_freeblks);
  do {
    while (blk != NULL && blk->mb_size < ((ssize_t) size)) {
      blk = LIST_NEXT(blk, mb_free_list);
    }

    if (blk == NULL) {
      return NULL;
    }

    // assert(blk->mb_size > 0);
    data_ptr = blk->mb_data;
    diff = align_ptr(&data_ptr, alignment);
    if ((ssize_t) (diff + size) <= blk->mb_size) {
      if (diff < sizeof(mem_block_t)) {
        blk = shift_block(arena, blk, diff);
      } else {
        blk = split_block(arena, blk, __containerof(data_ptr, mem_block_t, mb_data));
      }

      if (((size_t) blk->mb_size) - size > sizeof(mem_block_t)) {
        split_block(arena, blk, ((char*) data_ptr) + size);
      }

      LIST_REMOVE(blk, mb_free_list);
      blk->mb_size *= -1;
    } else {
      blk = LIST_NEXT(blk, mb_free_list);
      data_ptr = NULL;
    }
  } while (data_ptr == NULL);
  return data_ptr;
}

// merge once used block, now being freed
mem_block_t *merge_block(mem_arena_t *arena, mem_block_t *blk) {
  assert(blk->magic == 0xDEADC0DE);
  mem_block_t *next = LIST_NEXT(blk, mb_list);
  mem_block_t *prev = LIST_PREV(blk, &arena->ma_blocks, mem_block, mb_list);
  int prev_used, next_used;
  next_used = (next == NULL || next->mb_size < 0);
  prev_used = (prev == NULL || prev->mb_size < 0);

  blk->mb_size = - blk->mb_size;

  if (!prev_used && !next_used) {
    prev->mb_size += blk->mb_size + MEM_BLOCK_HDR_S;
    LIST_REMOVE(blk, mb_list);
    blk = prev;
    blk->mb_size += next->mb_size + MEM_BLOCK_HDR_S;
    LIST_REMOVE(next, mb_list);
    LIST_REMOVE(next, mb_free_list);
  } else if (!prev_used && next_used) {
    prev->mb_size += blk->mb_size + MEM_BLOCK_HDR_S;
    LIST_REMOVE(blk, mb_list);
    blk = prev;
  } else if (prev_used && !next_used) {
    blk->mb_size += next->mb_size + MEM_BLOCK_HDR_S;
    LIST_INSERT_BEFORE(next, blk, mb_free_list);
    LIST_REMOVE(next, mb_free_list);
    LIST_REMOVE(next, mb_list);
  } else if (prev_used && next_used) {
    insert_free_block(arena, blk);
  }
  return blk;
}

mem_block_t* shift_block(mem_arena_t* arena, mem_block_t *blk, size_t diff) {
  assert(diff < sizeof(mem_block_t));
  assert(sabs(blk->mb_size) > diff);
  assert(blk->magic == 0xDEADC0DE);
  if (diff == 0) return blk;

  mem_block_t *prev = LIST_PREV(blk, &arena->ma_blocks, mem_block, mb_list);
  ssize_t new_size = blk->mb_size - sgn(blk->mb_size) * diff;

  if (blk->mb_size > 0) {
    mem_block_t *prev_free;
    prev_free = LIST_PREV(blk, &arena->ma_freeblks, mem_block, mb_free_list);
    LIST_REMOVE(blk, mb_free_list);
    LIST_REMOVE(blk, mb_list);

    blk = (mem_block_t*) (((char*) blk) + diff);

    if (prev == NULL) {
      LIST_INSERT_HEAD(&arena->ma_blocks, blk, mb_list);
    } else {
      LIST_INSERT_AFTER(prev, blk, mb_list);
      prev->mb_size += sgn(prev->mb_size) * diff;
    }

    if (prev_free == NULL) {
      LIST_INSERT_HEAD(&arena->ma_freeblks, blk, mb_free_list);
    } else {
      LIST_INSERT_AFTER(prev_free, blk, mb_free_list);
    }
  } else {
    LIST_REMOVE(blk, mb_list);

    blk = (mem_block_t*) (((char*) blk) + diff);

    if (prev == NULL) {
      LIST_INSERT_HEAD(&arena->ma_blocks, blk, mb_list);
    } else {
      LIST_INSERT_AFTER(prev, blk, mb_list);
      prev->mb_size += sgn(prev->mb_size) * diff;
    }
  }

  blk->magic = 0xDEADC0DE;
  blk->mb_size = new_size;

  return blk;
}

mem_block_t* split_block(mem_arena_t* arena, mem_block_t *src, void *split) {
  mem_block_t *new_block = split;
  char *data_end = ((char*) src->mb_data) + sabs(src->mb_size);
  ssize_t src_size_sgn = sgn(src->mb_size);
  ssize_t diff;

  // check if the new block is at least just after the src
  assert(((char*) (new_block + 1)) <= data_end);
  assert(((char*) new_block) >= ((char*) (src + 1)));
  assert(src->magic == 0xDEADC0DE);

  diff = ((char*) split) - ((char *) src->mb_data);
  src->mb_size = src_size_sgn * diff;
  new_block->mb_size = data_end - ((char*) new_block->mb_data);
  new_block->magic = 0xDEADC0DE;

  LIST_INSERT_AFTER(src, new_block, mb_list);
  if (src_size_sgn > 0) {
    LIST_INSERT_AFTER(src, new_block, mb_free_list);
  } else {
    insert_free_block(arena, new_block);
  }
  return new_block;
}

void insert_free_block(mem_arena_t *arena, mem_block_t *blk) {
  assert(blk->magic == 0xDEADC0DE);
  assert(blk->mb_size > 0);
  mem_block_t *prev, *next;
  if (LIST_EMPTY(&arena->ma_freeblks)) {
    LIST_INSERT_HEAD(&arena->ma_freeblks, blk, mb_free_list);
  } else {
    prev = NULL;
    next = LIST_FIRST(&arena->ma_freeblks);
    while (next != NULL && next < blk) {
      prev = next;
      next = LIST_NEXT(prev, mb_free_list);
    }
    if (prev == NULL) {
      LIST_INSERT_HEAD(&arena->ma_freeblks, blk, mb_free_list);
    } else {
      LIST_INSERT_AFTER(prev, blk, mb_free_list);
    }
  }
}

/* UTILITY FUNCTIONS */

size_t sabs(ssize_t x) {
  return x >= 0 ? x : - x;
}

size_t align_ptr(void **ptr, size_t alignment) {
  size_t ptr_amc = ((size_t) (*ptr));
  size_t diff = alignment - (ptr_amc % alignment);
  diff = diff == alignment ? 0 : diff; // if it was aligned, keep it
  *ptr = (void *) (ptr_amc + diff);
  return diff;
}

__attribute__((constructor)) void init() {
  pthread_mutexattr_t    ma;
  pthread_mutexattr_init(&ma);
  pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&mem_ctrl.mutex, &ma);
  setbuf(stdout, NULL);
  printf("libmalloc initialized\n");
}
