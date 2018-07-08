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
#define MAX(x, y) ((x) > (y) ? (x) : (y))

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
#define PREV_INFO(bp) ((char *)(bp))
#define NEXT_INFO(bp) ((char *)(bp) + WSIZE)

/* Given block ptr bp, compute address of its predecessor and successor blocks */
#define PREV(bp) (*(char **)(bp))
#define NEXT(bp) (*(char **)(bp + WSIZE))

/* Given and pred/succ pointer p and a free block pointer ptr, set p to point to ptr */
#define SET(p, ptr) (*(unsigned int *)(p) = (unsigned int)(ptr))

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Pointer for the start of the heap */
static char *heap_ptr;

/* Pointer to the head of the free list */
static char *free_ptr;

static void *extend_heap(size_t words);
static void *find_fit(size_t size);
static void place(void *bp, size_t size);
static void *coalesce(void *bp);
static void remove_blk(void *bp);
static void insert_blk(void *bp);

static void *coalesce(void *bp)
{
    size_t pa = GET_ALLOC(FTRP(PREV_BLKP(bp))) || PREV_BLKP(bp) == bp;
    size_t na = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (pa && !na) {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        remove_blk(NEXT_BLKP(bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    else if (!pa && na) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp);
        remove_blk(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    else if (!pa && !na) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
            GET_SIZE(HDRP(NEXT_BLKP(bp)));
        remove_blk(PREV_BLKP(bp));
        remove_blk(NEXT_BLKP(bp));
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    insert_blk(bp);
    return bp;
}

static void insert_blk(void *bp)
{
    if (free_ptr == NULL) {
        free_ptr = bp;
        PREV(bp) = NULL;
        NEXT(bp) = NULL;
    }
    else {
        NEXT(bp) = free_ptr;
        PREV(bp) = NULL;
        PREV(free_ptr) = bp;
        free_ptr = (char *) bp;
    }
}

/**
 * extend_heap
 *
 * Takes a size to extend by, makes sure the size meets alignment requirements,
 * and requests additonal heap space. Called when the heap is intialized or 
 * when mm_malloc can't find a large enough free block.
 *
 * @param words : Number of words to extend the heap by.
 * @return : a pointer to the the newly freed block.
 */
static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE; //maintain alignment
    
    size = (size < MIN_SIZE) ? MIN_SIZE : size; //ensure size > 16
         
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
}

/**
 * remove
 *
 * Takes a pointer bp to a block in the free list and removes from the list.
 *
 * @param bp : A void pointer to a block that needs to be freed
 */
static void remove_blk(void *bp)
{
    if (PREV(bp) == NULL)
        free_ptr = NEXT(bp);
    else
        NEXT(PREV(bp)) = NEXT(bp);
    
    PREV(NEXT(bp)) = PREV(bp);
}

static void *find_fit(size_t size)
{
    void *ptr = free_ptr;
    while (ptr != NULL && GET_SIZE(HDRP(ptr)) < size)
        ptr = NEXT(ptr);

    return ptr;
}

/**
 * place
 *
 * Puts a block of size bytes at the freeblock pointed to by bp. Will split
 * the block if the remaining free bytes are greater than or equal to the
 * minimum block size.
 *
 * @param bp : pointer the free block
 * @param size : the number of bytes to place at bp
 */
static void place(void *bp, size_t size)
{
    size_t total_size = GET_SIZE(HDRP(bp));
    size_t difference = total_size - size;
    /* If the free block is to small to split */
    if ((difference) < MIN_SIZE) {
        PUT(HDRP(bp), PACK(total_size, 1));
        PUT(FTRP(bp), PACK(total_size, 1));
        remove_blk(bp);   
    }

    else {
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
        remove_blk(bp);
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(difference, 0));
        PUT(FTRP(bp), PACK(difference, 0));
        coalesce(bp);
    }
                
}



/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{

    /* Create the intial empty heap */
    if ((heap_ptr = mem_sbrk(WSIZE * 4)) == (void *)-1)
        return -1;

    PUT(heap_ptr, 0); //Alignment padding
    PUT(heap_ptr + (1 * WSIZE), PACK(DSIZE, 1)); //Prologue header
    PUT(heap_ptr + (2 * WSIZE), PACK(DSIZE, 1)); //Prologue footer
    PUT(heap_ptr + (3 * WSIZE), PACK(0, 1)); //Epilogue header
    //heap_ptr += DSIZE;
    free_ptr = heap_ptr + DSIZE;
        
    if(extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t extend;
    char *bp;

    if (size == 0)
        return NULL;
    
    if (size < MIN_SIZE) 
        size = MIN_SIZE;
    else //Round up to nearest mult of 8 for alignment then add 8 for header/footer
        size = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
    
    if ((bp = find_fit(size)) != NULL) {
        place(bp, size);
        return bp;
    }
    //If no fit, extend the heap by the larger of size and chunksize then place
    extend = MAX(size, CHUNKSIZE);
    
    if ((bp = extend_heap(extend/WSIZE)) == NULL)
        return NULL;
    
    place(bp, size);
    
    return bp;

}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    if (ptr == NULL)
        return;

    size_t size = GET_SIZE(HDRP(ptr));
    
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));

    coalesce(ptr);
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












