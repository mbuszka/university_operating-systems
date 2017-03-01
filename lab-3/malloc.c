#include "arena.h"
#include "malloc.h"
#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define TRACE 1

/* global memory controller */
mem_ctrl_t mem_ctrl = { LIST_HEAD_INITIALIZER(mem_ctrl), NULL, PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP };

void *malloc(size_t size) {
  pthread_mutex_lock(&mem_ctrl.mutex);
  void *ptr;
  int res = posix_memalign(&ptr, sizeof(void*), size);
  if (ptr == NULL) {
    setbuf(stdout, NULL);
    printf("memalign returned %d\n", res);
    pthread_mutex_unlock(&mem_ctrl.mutex);
    return NULL;
  }
  pthread_mutex_unlock(&mem_ctrl.mutex);
  return ptr;
}

void *calloc(size_t count, size_t size) {
  pthread_mutex_lock(&mem_ctrl.mutex);
  void *ptr;
  int res = posix_memalign(&ptr, sizeof(void*), size * count);
  if (ptr == NULL) {
    setbuf(stdout, NULL);
    printf("memalign returned %d\n", res);
    pthread_mutex_unlock(&mem_ctrl.mutex);
    return NULL;
  }
  char *beg = (char*) ptr;
  for (size_t i=0; i<size * count; i++) {
    beg[i] = 0;
  }
  pthread_mutex_unlock(&mem_ctrl.mutex);
  return ptr;
}

int posix_memalign(void **memptr, size_t alignment, size_t size) {
  pthread_mutex_lock(&mem_ctrl.mutex);
  if (TRACE) fprintf(stderr, "malloc : %s\n", __func__);
  mem_arena_t *arena;
  void *ptr;
  size_t diff;
  if (size == 0) {
    pthread_mutex_unlock(&mem_ctrl.mutex);
    *memptr = NULL;
    return 0;
  }
  if (alignment % sizeof(void *) != 0 || !is_power_of_two(alignment)) {
    pthread_mutex_unlock(&mem_ctrl.mutex);
    return EINVAL;
  }

  if (size < 16) size = 16;
  if (size % DEFAULT_ALIGNMENT != 0) size += DEFAULT_ALIGNMENT - (size % DEFAULT_ALIGNMENT);

  if (size > BIG_ARENA_TRESHOLD || alignment > EXCLUSIVE_ARENA_TRESHOLD) {
    ptr = mem_ctrl.mc_next_addr;
    diff = alignment < PAGESIZE ? PAGESIZE : alignment;
    arena = spawn_arena(size + diff);
    ptr = allocate_block(arena, size, alignment);
    assert(ptr != NULL);
    assert(*(((uint64_t*) ptr) - 1) == 0xDEADC0DE);
    *memptr = ptr;
    pthread_mutex_unlock(&mem_ctrl.mutex);
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
    assert(*(((uint64_t*) ptr) - 1) == 0xDEADC0DE);
  }
  *memptr = ptr;
  pthread_mutex_unlock(&mem_ctrl.mutex);
  return 0;
}

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
    if (ptr_l < ((size_t) blk) + sabs(blk->mb_size)) {
      return blk;
    }
  }
  return NULL;
}

void free(void *ptr) {
  if (ptr == NULL) {
    return;
  }
  pthread_mutex_lock(&mem_ctrl.mutex);

  if (TRACE) fprintf(stderr, "malloc : %s\n", __func__);

  mem_arena_t *arena;
  mem_block_t *blk;


  arena = find_arena(ptr);
  blk = __containerof(ptr, mem_block_t, mb_data);

  if (blk == NULL) {
    // we didn't allocate this pointer, so we have memory corruption
    pthread_mutex_unlock(&mem_ctrl.mutex);
    return;
  }
  assert(*(((uint64_t*) ptr) - 1) == 0xDEADC0DE);
  if (blk->mb_size > 0) {
    // we are trying to free unused block, it's fine
    pthread_mutex_unlock(&mem_ctrl.mutex);
    return;
  }

  blk = merge_block(arena, blk);
  assert(blk->mb_size > 0);
  blk = LIST_FIRST(&arena->ma_blocks);
  if (blk->mb_size + MEM_BLOCK_HDR_S == arena->ma_size) {
    remove_arena(arena);
  }

  pthread_mutex_unlock(&mem_ctrl.mutex);
}

void *realloc(void *ptr, size_t size) {
  if (TRACE) fprintf(stderr, "malloc : %s\n", __func__);
  pthread_mutex_lock(&mem_ctrl.mutex);
  if (size == 0) {
    free(ptr);
    pthread_mutex_unlock(&mem_ctrl.mutex);
    return NULL;
  }

  if (ptr == NULL) {
    ptr = malloc(size);
    pthread_mutex_unlock(&mem_ctrl.mutex);
    return ptr;
  }

  if (size < 16) size = 16;
  if (size % DEFAULT_ALIGNMENT != 0) size += DEFAULT_ALIGNMENT - (size % DEFAULT_ALIGNMENT);

  mem_arena_t *arena;
  mem_block_t *blk, *next;
  arena = find_arena(ptr);
  blk   = __containerof(ptr, mem_block_t, mb_data);
  if (blk->magic != 0xDEADC0DE) {
    dump_arena(arena, blk);
    errno = ENOMEM;
    return NULL;
  }


  size_t blk_size = - blk->mb_size;
  assert(blk->mb_size < 0);

  if (size <= blk_size && blk_size - size <= sizeof(mem_block_t)) {
    // do nothing
    assert(ptr != NULL);
    pthread_mutex_unlock(&mem_ctrl.mutex);
    return ptr;
  }

  if (size <= blk_size && blk_size - size > sizeof(mem_block_t)) {
    split_block(arena, blk, ((char*) blk->mb_data) + size);
    assert(ptr != NULL);
    pthread_mutex_unlock(&mem_ctrl.mutex);
    return ptr;
  }

  next = LIST_NEXT(blk, mb_list);
  blk_size += next->mb_size + MEM_BLOCK_HDR_S;
  if (next->mb_size < 0 || blk_size < size) {
    void* newptr = malloc(size);
    memcpy(newptr, ptr, - blk->mb_size);
    free(ptr);
    pthread_mutex_unlock(&mem_ctrl.mutex);
    assert(newptr != NULL);
    return ptr;
  } else {
    char * split_ptr = ((char*) ptr) + size;
    blk->mb_size = - blk_size;
    LIST_REMOVE(next, mb_list);
    LIST_REMOVE(next, mb_free_list);
    if (blk_size - size >= sizeof(mem_block_t)) {
      split_block(arena, blk, split_ptr);
    }
    assert(ptr != NULL);
    pthread_mutex_unlock(&mem_ctrl.mutex);
    return ptr;
  }
  pthread_mutex_unlock(&mem_ctrl.mutex);
  return ptr;
}
