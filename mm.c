/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "506 Team 2",
    /* First member's full name */
    "김예람",
    /* First member's email address */
    "kyeram522@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/*01. define basic constant and macro*/

/* single word (4) or double word (8) alignment */
#define WSIZE 4 // 64bits 기준 word size 정의. header과 footer의 크기(4bytes)
#define DSIZE 8 // Double Word size
#define CHUNKSIZE (1 << 12) /*Extend heap by this amount(bytes)*/

#define MAX(x,y) ((x) > (y) ? (x) : (y))

/*Read a size and allocated bit into a word*/
#define PACK(size, alloc) ((size) | (alloc)) // |(or) 비트 연산을 통해서size와 alloc 비트를 합친다??( 나중에 디버깅 돌려보기.. )

/*Read and write a word at address p*/
#define GET(p) (*(unsigned int*)(p)) // 들어오는 값이 void 자료형이기 때문에 캐스팅을 통해 값을 넣어준다. P가 참조하는 word를 읽어서 리턴
#define PUT(p, val) (*(unsigned int*)(p) = (val))

/*Read the size and allocated field from address p */
#define GET_SIZE(p) (GET(p) & ~0x7) //마스킹을 이용해서 헤더 또는 풋터의 size와 allocated 리턴
#define GET_ALLOC(p) (GET(p) & 0x1)

/*Given block ptr bp, compute address of its header and footer*/
#define HDRP(bp) ((char*)(bp) - WSIZE) //블록 포인터를 읽어서 header 를 가르키는 포인터 반환
#define FTRP(bp) ((char*)(bp)+ GET_SIZE(HDRP(bp))- DSIZE) // 블록 포인터를 읽어서 size만큼 더하고 2word(headr+ footer bytes)를 빼서 footer을 가리키는 포인터 반환

/*Given block ptr bp, compute address of next and previous block */
#define NEXT_BLKP(bp) ((char*)(bp) + GET_SIZE((char*)(bp) - WSIZE))
#define PREV_BLKP(bp) ((char*)(bp) - GET_SIZE((char*)(bp) - DSIZE))

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

static void * extend_heap(size_t);
static void * coalesce(void*);
static void place(void *, size_t );
static void *find_fit(size_t );
static void* heap_listp;

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    
    /*Create the initial empty heap*/
    if((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;

    PUT(heap_listp, 0);
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE,1)); /*prolog header*/
    PUT(heap_listp  + (2*WSIZE), PACK(DSIZE,1)); /*prolog footer*/
    PUT(heap_listp + (3*WSIZE), PACK(0,1)); /*apilog header*/

    heap_listp += (2*WSIZE); /*prolog footer를 가리키면서 한 워드만 가면 payload의 시작이다.*/


    /*extend the empty heap with a free block of CHUNKSIZE bytes*/
    if(extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;

    return 0;
}




static void * extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /*Allocate an even number of words to maintaion alignment*/
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    /*Initialize free block header/footer and the epilogue header*/
    PUT(HDRP(bp), PACK(size,0)); /*Free block header*/
    PUT(FTRP(bp), PACK(size,0)); /*Free block footer*/
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1)); /*New epilogue header*/

    /*Coalesce if the previous block was free*/
    return coalesce(bp);
}

static void * coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if(prev_alloc && next_alloc){ /*CASE 1 */
        return bp;
    }

    else if(prev_alloc && !next_alloc){ /*CASE 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size,0)); //병합 후 크기가 바뀌었으므로
        PUT(FTRP(bp), PACK(size,0)); //FTRP(bp) 새 블록의 푸터임
    }

    else if(!prev_alloc && next_alloc){ /*CASE 3*/
        size += GET_SIZE(FTRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size,0)); //FTRP(bp)는 현재 블록의 푸터
        PUT(HDRP(PREV_BLKP(bp)), PACK(size,0)); //이전 블록의 헤더를 갱신
       

        bp = PREV_BLKP(bp);
    }

    else{ /*CASE 4*/
        size += ( GET_SIZE(FTRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp))) );
        PUT(HDRP(PREV_BLKP(bp)), PACK(size,0)); 
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size,0));

        bp = PREV_BLKP(bp);
    }

    return bp;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    /*Adjusted block size*/
    size_t asize;
    /*Amount of extend heap if no fit*/
    size_t extendsize;
    char *bp;

    /*Ignore spurious requests*/
    if(size <= 0)
        return NULL;

    /*Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE) // 정렬조건을 만족시키기 위해 8바이트로 해주고, header,footer 의 8바이트도 제공
        asize = 2*DSIZE;
    else //8바이트를 넘는 요청에 대해서 오버헤드 바이트를 추가하고, 인접 8의 배수로 반올림한다.
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE );

    /*Search the free list for a fit*/
    if((bp = find_fit(asize)) != NULL){
        place(bp, asize);
        return bp;
    }
    /*No fit found. Get more memory and place the block*/
    extendsize = MAX(asize, CHUNKSIZE);
    if((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
        
    place(bp, asize);
    
    return bp;
}

static void *find_fit(size_t asize){ /*가용 free 리스트를 처음부터 검색해서 크기가 맞는 첫번째 가용 블록을 선택*/
    void *bp =  NEXT_BLKP(heap_listp) ;
    //void *bp =  mem_heap_lo() + 2*WSIZE ;

    while (GET_SIZE(HDRP(bp)) > 0 )
    {
        if(GET_ALLOC(HDRP(bp)) == 0 &&  asize <= GET_SIZE(HDRP(bp))) return (void*)bp;

        bp = NEXT_BLKP(bp);
    }

    return NULL;
    
}


static void place(void *bp, size_t asize){ //asize = 원래 새로 들어온 아이의 사이즈
    /*새로 할당된 아이의 footer 갱신*/
    size_t size = GET_SIZE(HDRP(bp)); //현재 쓰고있는 공간 사이즈

    if((size - asize) >= (2*DSIZE)){//블록의 크기는 2워드(8바이트)의 배수여야한다.

        PUT(HDRP(bp), PACK(asize,1)); 
        PUT(FTRP(bp), PACK(asize,1)); 
    
        PUT(HDRP(NEXT_BLKP(bp)), PACK(size - asize,0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size - asize,0));
    }
    else{//남은 공간이 8바이트의 배수보다 작다면, 
        PUT(HDRP(bp),PACK(size,1));
        PUT(FTRP(bp),PACK(size,1));
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size  = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size,0));
    PUT(FTRP(bp), PACK(size,0));

    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    if(ptr == NULL)
    {
        return mm_malloc(size);
    }

    if(size == 0)
    {
        mm_free(ptr);
        return NULL;
    }
        
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;

    copySize = GET_SIZE(ptr) - DSIZE;

    if (size < copySize)
      copySize = size;
      

    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);

    return newptr;
}














