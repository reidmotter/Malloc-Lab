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
    "Team Reid",
    /* First member's full name */
    "Reid Motter",
    /* First member's email address */
    "motterra@appstate.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Basic constants and macros */
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12)
#define MIN_SIZE 16
#define NUM_CLASSES 20

/* Read and write a word at address p */
#define GET(p)  (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_TAG(p) (GET(p) & 0x2)

/* Given block ptr bp, computer address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* Given block ptr bp, compute address of its predecessor and successor info */
#define PRED_INFO(bp) ((char *)(bp))
#define SUCC_INFO(bp) ((char *)(bp) + WSIZE)

/* Given block ptr bp, compute address of its predecessor and successor blocks */
#define PRED(bp) (*(char **)(bp))
#define SUCC(bp) (*(char **)(SUCC_INFO(ptr)))

/* Given and pred/succ pointer p and a free block pointer ptr, set p to point to ptr */
#define SET(p, ptr) (*(unsigned int *)(p) = (unsigned int)(ptr))

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Pointer for the start of the heap */
static char *heap_listp;

/* Array to hold the different size classes */
void *free_list[NUM_CLASSES];

static void *extend_heap(size_t words);
static void insert(void *bp, size_t size);

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    size = ALIGN(words);

    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    insert(bp, size);

    return coalesce(bp);
}

static void insert(void *bp, size_t size)
{
    int count = 0;
    void *pred = NULL;
    void *succ = NULL;

    while (count < NUM_CLASSES -1 && size > 1)
    {
        size = size >> 1;
        count++;
    }

    succ = free_list[count];

    while (succ != NULL && size < GET_SIZE(HDRP(succ)))
    {
        pred = succ;
        succ = PRED(succ);
    }
    
    if(succ != NULL)
    {
        SET(PRED_INFO(bp), succ);
        SET(SUCC_INFO(succ), bp);
        SET(SUCC_INFO(bp), pred);
    }

    
}
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    for (int i = 0; i < NUM_CLASSES; i++)
        freelist[i] = NULL;

    /* Create the intial empty heap */
    if ((heap_listp = mem_sbrk(WSIZE * 4)) == (void *)-1)
        return -1;

    PUT(heap_listp, 0); //Alignment padding
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); //Prologue header
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); //Prologue footer
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1)); //Epilogue header
    heap_listp += (2 * WSIZE);

    if(extended_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
        return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}












