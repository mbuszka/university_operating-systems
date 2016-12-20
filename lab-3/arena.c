#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "arena.h"

extern mem_ctrl_t mem_ctrl;

mem_arena_t *spawn_arena(size_t size) {
  // mutex lock
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

void *allocate_block(mem_arena_t *arena, size_t size, size_t alignment) {
  mem_block_t *blk;
  void *data_ptr;

  blk = LIST_FIRST(&arena->ma_freeblks);
  do {
    while (blk != NULL && blk->mb_size < ((ssize_t) size)) {
      blk = LIST_NEXT(blk, mb_free_list);
    }
    if (blk == NULL) {
      return NULL;
    } else {
      data_ptr = split_block(blk, size, alignment);
    }
  } while (data_ptr == NULL);
  return data_ptr;
}

size_t align_ptr(void **ptr, size_t alignment) {
  size_t ptr_amc = ((size_t) (*ptr));
  size_t diff = alignment - (ptr_amc % alignment);
  diff = diff == alignment ? 0 : diff; // if it was aligned, keep it
  *ptr = (void *) (ptr_amc + diff);
  return diff;
}

/* Sets passed pointer to be properly aligned, returns real size of a block */
ssize_t align_block_data(void** beginning, size_t size, size_t alignment) {
  size += align_ptr(beginning, alignment);
  return (ssize_t) size;
}

// only for free blocks
void* split_block(mem_block_t *src, size_t size, size_t alignment) {
  void *data_ptr;
  ssize_t real_size;
  mem_block_t *new_block;

  data_ptr = &src->mb_data;
  real_size = align_block_data(data_ptr, size, alignment);

  if (real_size > src->mb_size) return NULL;

  if (real_size + ((ssize_t) sizeof(mem_block_t)) < src->mb_size) {
    new_block = (mem_block_t *) (((size_t) data_ptr) + real_size);
    new_block->mb_size = src->mb_size - real_size - MEM_BLOCK_HDR_S;
    src->mb_size = -real_size;
    LIST_INSERT_AFTER(src, new_block, mb_list);
    LIST_INSERT_AFTER(src, new_block, mb_free_list);
    LIST_REMOVE(src, mb_free_list);
  }
  return data_ptr;
}

void insert_free_block(mem_arena_t *arena, mem_block_t *blk) {
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

// merge once used block, now being freed
mem_block_t *merge_block(mem_arena_t *arena, mem_block_t *blk) {
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

void dump_arena(mem_arena_t *arena, mem_block_t *highlight) {
  mem_block_t *el;
  int i=0;
  printf("Dumping arena\n\tbeginning : %lx\n\tsize : %lu\n"
        , (size_t) arena
        , arena->ma_size
        );
  LIST_FOREACH(el, &arena->ma_blocks, mb_list) {
    printf("%sBlock %d, beginning : %lx, size : %ld\n"
          , el == highlight ? " >>\t" : "\t"
          , i
          , (size_t) el
          , el->mb_size
          );
    i++;
  }
}
