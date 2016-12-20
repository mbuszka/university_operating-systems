#include "arena.h"
#include "malloc.h"
#include <errno.h>
#include <assert.h>
#include <stdio.h>

#define abs(number) ((number) > 0 ? (number) : (- (number)))

/* global memory controller */
mem_ctrl_t mem_ctrl = { LIST_HEAD_INITIALIZER(mem_ctrl), NULL };

void *malloc(size_t size) {
  // printf("hello\n");
  void *ptr;
  int res = posix_memalign(&ptr, sizeof(void*), size);
  if (res != 0) return NULL;
  return ptr;
}

void *calloc(size_t count, size_t size) {
  void *ptr;
  int res = posix_memalign(&ptr, sizeof(void*), size * count);
  if (res != 0) return NULL;
  return ptr;
}

int posix_memalign(void **memptr, size_t alignment, size_t size) {
  mem_arena_t *arena;
  void *ptr;
  size_t diff;

  if (alignment % sizeof(void *) != 0 || !is_power_of_two(alignment)) {
    return EINVAL;
  }

  if (size > BIG_ARENA_TRESHOLD || alignment > EXCLUSIVE_ARENA_TRESHOLD) {
    ptr = mem_ctrl.mc_next_addr;
    ptr = (void *) (((size_t) ptr) + sizeof(mem_arena_t) + MEM_BLOCK_HDR_S);
    diff = align_ptr(&ptr, alignment);

    if (diff > PAGESIZE) {
      mem_ctrl.mc_next_addr += diff / PAGESIZE;
      diff %= PAGESIZE;
    }

    arena = spawn_arena(size + diff);
    ptr = allocate_block(arena, size, alignment);
    assert(ptr != NULL);
    *memptr = ptr;
    return 0;
  }

  ptr = NULL;
  arena = LIST_FIRST(&mem_ctrl.mc_arenas);
  do {
    if (arena == NULL) break;
    ptr = allocate_block(arena, size, alignment);
    arena = LIST_NEXT(arena, ma_list);
  } while (ptr == NULL);

  if (ptr == NULL) {
    arena = spawn_arena(DEFAULT_ARENA_SIZE);
    ptr = allocate_block(arena, size, alignment);
  }

  *memptr = ptr;
  return 0;
}

// void *realloc(void* ptr, size_t size) {
//
// }

mem_arena_t *find_arena(void *ptr) {
  mem_arena_t *arena, *tmp;
  size_t begin, ptr_l;
  ptr_l = (size_t) ptr;

  LIST_FOREACH_SAFE(arena, &mem_ctrl.mc_arenas, ma_list, tmp) {
    begin = (size_t) arena;
    if (ptr_l > begin && ptr_l < begin + arena->ma_size + sizeof(mem_arena_t)) {
      return arena;
    }
  }
  return NULL;
}

mem_block_t *find_block(mem_arena_t* arena, void* ptr) {
  size_t begin, ptr_l;
  ptr_l = (size_t) ptr;
  begin = (size_t) arena;

  mem_block_t *blk, *tmp_b;
  LIST_FOREACH_SAFE(blk, &arena->ma_blocks, mb_list, tmp_b) {
    if (ptr_l < ((size_t) blk) + abs(blk->mb_size)) {
      return blk;
    }
  }
  return NULL;
}

void free(void *ptr) {
  mem_arena_t *arena;
  mem_block_t *blk;

  arena = find_arena(ptr);
  blk = find_block(arena, ptr);

  if (blk == NULL) {
    // we didn't allocate this pointer, so we have memory corruption
    return;
  }
  if (blk->mb_size > 0) {
    // we are trying to free unused block, it's an error
    return;
  }

  merge_block(arena, blk);
  blk = LIST_FIRST(&arena->ma_blocks);
  if (blk->mb_size + MEM_BLOCK_HDR_S == arena->ma_size) {
    remove_arena(arena);
  }
}

// void *realloc(void *ptr, size_t size) {
//   if (size == 0) {
//     free(ptr);
//     return NULL;
//   }
//
//   mem_arena_t *arena;
//   mem_block_t *blk, *next, *last;
//   arena = find_arena(ptr);
//   blk   = find_block(arena, ptr);
//
//   size_t src_size = ((size_t) ptr) - ((size_t) blk->mb_data);
//
//   if (size <= src_size && src_size - size > sizeof(mem_block_t)) {
//     mem_block_t *nblk = (mem_block_t*) (((size_t) blk->mb_data) + size);
//     nblk->mb_size = src_size - size - MEM_BLOCK_HDR_S;
//     LIST_INSERT_AFTER(blk, nblk, mb_list);
//     insert_free_block(arena, blk);
//     return ptr;
//   }
//
//   if (size <= src_size && src_size - size <= sizeof(mem_block_t)) {
//     // do nothing
//     return ptr;
//   }
//
//   size_t space_needed = size - src_size;
//
//   last = blk;
//   next = LIST_NEXT(blk, mb_list);
//   while (next != NULL
//       && next->mb_size > 0
//       && space_needed > next->mb_size + MEM_BLOCK_HDR_S) {
//         space_needed -= next->mb_size + MEM_BLOCK_HDR_S;
//         last = next;
//         next = LIST_NEXT(last, mb_list);
//   }
//   if (last->mb_size > 0 && space_needed < last->mb_size + MEM_BLOCK_HDR_S) {
//     // remove all prior blks
//     next = LIST_NEXT(blk, mb_list);
//     do {
//
//     }
//   }
//
// }

void dump_all() {
  mem_arena_t *ar;
  LIST_FOREACH(ar, &mem_ctrl.mc_arenas, ma_list) {
    if (ar == NULL) return;
    dump_arena(ar, NULL);
  }
}
