#define _MALLOC_INTERNAL
#include <stdio.h>
#include <assert.h>
#include "arena.h"
#include "malloc.h"

void test_posix_memalign() {
  void *p1, *p2, *p3, *p4;
  dump_all();
  // getchar();
  posix_memalign(&p1, 8, 10000);
  dump_all();
  printf("----------\n");
  // getchar();

  posix_memalign(&p2, 8, 400);
  dump_all();
  printf("----------\n");
  // getchar();

  posix_memalign(&p3, 1024, 8000);
  dump_all();
  printf("----------\n");
  // getchar();

  posix_memalign(&p4, 64, 400000);
  dump_all();
  printf("----------\n");
  // getchar();

  printf("freeing p1\n");
  free(p1);
  dump_all();
  printf("----------\n");
  // getchar();

  printf("freeing p2\n");
  free(p2);
  dump_all();
  printf("----------\n");
  // getchar();

  printf("freeing p3\n");
  free(p3);
  dump_all();
  printf("----------\n");
  // getchar();

  printf("freeing p4\n");
  free(p4);
  dump_all();
  printf("----------\n");
  // getchar();
}

void test_blocks(mem_arena_t *arena, size_t block_size, size_t count) {
  for (size_t i=0; i<count; i++) {
    allocate_block(arena, block_size, 8);
  }
  dump_arena(arena, NULL);
  mem_block_t *blk;
  blk = LIST_FIRST(&arena->ma_blocks);
  blk = LIST_NEXT(blk, mb_list);
  blk = merge_block(arena, blk);
  dump_arena(arena, blk);
  blk = LIST_NEXT(blk, mb_list);
  blk = merge_block(arena, blk);
  dump_arena(arena, blk);
  blk = LIST_NEXT(blk, mb_list);
  blk = LIST_NEXT(blk, mb_list);
  merge_block(arena, blk);
  dump_arena(arena, blk);
  blk = LIST_PREV(blk, &arena->ma_blocks, mem_block, mb_list);
  blk = merge_block(arena, blk);
  dump_arena(arena, blk);
  blk = LIST_FIRST(&arena->ma_blocks);
  blk = merge_block(arena, blk);
  dump_arena(arena, blk);
  blk = LIST_NEXT(blk, mb_list);
  blk = merge_block(arena, blk);
  dump_arena(arena, blk);
  blk = LIST_NEXT(blk, mb_list);
  blk = merge_block(arena, blk);
  dump_arena(arena, blk);
}

int main() {
  setbuf(stdout, NULL);
  printf("%lu\n", sizeof(mem_arena_t));
  test_posix_memalign();

  return 0;
}
