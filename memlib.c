/*
 * memlib.c - a module that simulates the memory system.  Needed because it 
 *            allows us to interleave calls from the student's malloc package 
 *            with the system's malloc package in libc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>

#include "memlib.h"
#include "config.h"

/* private variables */
static char *mem_start_brk;  /* points to first byte of heap */ /*mem start_brk과 mem brk사이의 바이트들은 할당된 가상 메모리를 나타낸다*/
static char *mem_brk;        /* points to last byte of heap */   /*mem_brk 다움에 오는 바이트들은 미할당 가상메모리를 나타낸다.*/
static char *mem_max_addr;   /* largest legal heap address */ 

/* 
 * mem_init - initialize the memory system model
 */
void mem_init(void) /*힙에 가용한 가상메모리를 더 큰 더블 워드로 정렬된 바이트의 배열로 모델한 것*/
{
    /* allocate the storage we will use to model the available VM */
    if ((mem_start_brk = (char *)malloc(MAX_HEAP)) == NULL) {
	fprintf(stderr, "mem_init_vm: malloc error\n");
	exit(1);
    }

    mem_max_addr = mem_start_brk + MAX_HEAP;  /* max legal heap address */
    mem_brk = mem_start_brk;                  /* heap is empty initially */
}

/* 
 * mem_deinit - free the storage used by the memory system model
 */
void mem_deinit(void)
{
    free(mem_start_brk);
}

/*
 * mem_reset_brk - reset the simulated brk pointer to make an empty heap
 */
void mem_reset_brk()
{
    mem_brk = mem_start_brk;
}

/* 
 * mem_sbrk - simple model of the sbrk function. Extends the heap 
 *    by incr bytes and returns the start address of the new area. In
 *    this model, the heap cannot be shrunk.
 */
void *mem_sbrk(int incr) //heap을 확장하는 함수. incr bytes 만큼 크기를 늘리고 새로운 주소의 위치를 반환한다.
{
    char *old_brk = mem_brk; //mem_brk 현재 힙의 마지막 포인터를 지역변수에 저장해둔다.

    if ( (incr < 0) || ((mem_brk + incr) > mem_max_addr)) { // 추가하려는 바이트 크기가 0보다 작거나, 현재 위치에서 추가할 바이트를 합한 주소가 힙의 최대 크기보다 크다면 예외처리.
	errno = ENOMEM;
	fprintf(stderr, "ERROR: mem_sbrk failed. Ran out of memory...\n");
	return (void *)-1; 
    }
    mem_brk += incr; //incr 만큼 주소 증가 -> 미할당 가상메모리의 주소를 incr 만큼 증가 시킨다.
    return (void *)old_brk; //시작 포인트
}

/*
 * mem_heap_lo - return address of the first heap byte
 */
void *mem_heap_lo()
{
    return (void *)mem_start_brk;
}

/* 
 * mem_heap_hi - return address of last heap byte
 */
void *mem_heap_hi()
{
    return (void *)(mem_brk - 1);
}

/*
 * mem_heapsize() - returns the heap size in bytes
 */
size_t mem_heapsize() 
{
    return (size_t)(mem_brk - mem_start_brk);
}

/*
 * mem_pagesize() - returns the page size of the system
 */
size_t mem_pagesize()
{
    return (size_t)getpagesize();
}
