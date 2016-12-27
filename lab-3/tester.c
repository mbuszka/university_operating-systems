#define _MALLOC_INTERNAL
#include <stdio.h>
#include <assert.h>
#include "arena.h"
#include "malloc.h"

void test_1() {
  int n = 100;
  int* ptrs[n];
  for (int i=0; i<n; i++) {
    ptrs[i] = malloc(sizeof(int));
    *ptrs[i] = i;
  }
  for (int i=0; i<n; i++) {
    assert(i == *ptrs[i]);
  }
  for (int i=0; i<n; i++) {
    free(ptrs[i]);
  }
  printf("shouldbe clear\n");
  dump_all();
  printf("----\n");
}

void test_2() {
  int n = 100;
  char* ptrs[n];
  for (int i=1; i<n; i++) {
    ptrs[i] = malloc(i+1);
    (ptrs[i])[0] = 'a';
    (ptrs[i])[i] = 'e';
  }
  // dump_all();
  for (int i=1; i<n; i++) {
    assert((ptrs[i])[0] == 'a');
    assert((ptrs[i])[i] == 'e');
  }

  for (int i=1; i<n; i++) {
    free(ptrs[i]);
  }
  printf("shouldbe clear\n");
  dump_all();
  printf("----\n");
}

void test_3() {
  int n = 100;
  char* ptrs[n];
  for (int i=1; i<n; i++) {
    ptrs[i] = malloc(100000);
    (ptrs[i])[0] = 'a';
    (ptrs[i])[i] = 'e';
  }
  for (int i=1; i<n; i++) {
    assert((ptrs[i])[0] == 'a');
    assert((ptrs[i])[i] == 'e');
  }
  // dump_all();
  for (int i=1; i<n; i++) {
    free(ptrs[i]);
  }
  printf("shouldbe clear\n");
  dump_all();
  printf("----\n");
}

void test_small_allocations() {
  dump_all();
  char* ptr = malloc(sizeof(char));
  dump_all();
  free(ptr);
}

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

void test_blocks() {
  mem_arena_t *arena = spawn_arena(1000);
  size_t block_size = 100;
  size_t count = 7;
  for (size_t i=0; i<count; i++) {
    allocate_block(arena, block_size, 8);
    dump_arena(arena, NULL);
    printf("---------\n");
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
  printf("shouldbe clear");
  dump_arena(arena, blk);
  printf("-----");
}

void test_malloc() {
  #include <stdlib.h>
  #include <time.h>
  srand(time(NULL));
  int n = 10000;
  char *mem[n];
  char tofree[n];
  for (int i=0; i<n; i++) {
    int s = 100 + rand() % 10000;
    mem[i] = malloc(s);
    for (int j=0; j<s; j++) {
      mem[i][j] = 'a';
    }
    tofree[i] = 1;
  }
  int k;
  for (int i=0; i<n; i++) {
    k = rand() % n;
    if (tofree[k] == 0) {
      int s = 100 + rand() % 10000;
      mem[k] = malloc(s);
      for (int j=0; j<s; j++) {
        mem[k][j] = 'a';
      }
      tofree[k] = 1;
    } else {
      free(mem[k]);
      tofree[k] = 0;
    }
  }
  for (int i=0; i<n; i++) {
    if (tofree[i]) {
      free(mem[i]);
    }
  }
  dump_all();
}

void spec_malloc() {
  char *ptr = malloc(1024);
  free(ptr);
  dump_all();
}

int main() {
  // setbuf(stdout, NULL);
  // printf("%lu\n", sizeof(mem_arena_t));
  // test_blocks();
  // test_posix_memalign();
  // // test_small_allocations();
  // test_1();
  // test_2();
  // test_3();
  test_malloc();

  return 0;
}
