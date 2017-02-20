/*
 *    NAME: VAIBHAV PATEL
 *    EMAIL ID: 201401222@daiict.ac.in
 * Simple, 32-bit and 64-bit clean allocator based on an implicit free list,
 * first fit placement, and boundary tag coalescing, as described in the
 * CS:APP2e text.  Blocks are aligned to double-word boundaries.  This
 * yields 8-byte aligned blocks on a 32-bit processor, and 16-byte aligned
 * blocks on a 64-bit processor.  However, 16-byte alignment is stricter
 * than necessary; the assignment only requires 8-byte alignment.  The
 * minimum block size is four words.
 *
 * This allocator uses the size of a pointer, e.g., sizeof(void *), to
 * define the size of a word.  This allocator also uses the standard
 * type uintptr_t to define unsigned integers that are the same size
 * as a pointer, i.e., sizeof(uintptr_t) == sizeof(void *).
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "memlib.h"
#include "mm.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "TooDifficult",
    /* First member's full name */
    "Vaibhav Patel",
    /* First member's email address */
    "vaibhav29.07.97@gmail.com",
    /* Second member's full name (leave blank if none) */
    "Tanmay Patel",
    /* Second member's email address (leave blank if none) */
     "tanmaypatel273@gmail.com"
};

/* Basic constants and macros: */
#define WSIZE      8              /* Word and header/footer size (bytes) */
#define DSIZE      (2 * WSIZE)    /* Doubleword size (bytes) */
#define CHUNKSIZE  (1 << 12)      /* Extend heap by this amount (bytes) */
#define NO_OF_SEGREGATED_LISTS 10
#define MAX(x, y)  ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word. */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p. */
#define GET(p)       (*(uintptr_t *)(p))
#define PUT(p, val)  (*(uintptr_t *)(p) = (val))

/* Read the size and allocated fields from address p. */
#define GET_SIZE(p)   (GET(p) & ~(DSIZE - 1))
#define GET_ALLOC(p)  (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer. */
#define HDRP(bp)  ((char *)(bp) - WSIZE)
#define FTRP(bp)  ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks. */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* Given free block ptr bp, compute address of next and previous free blocks in explicit free list. */
#define NEXT_LIST_BLKP(bp)   (*(void **)(bp + WSIZE))               
#define PREV_LIST_BLKP(bp)   (*(void **)(bp))

/* Given free block ptr bp, set address of next and previous free blocks in explicit free list. */
#define SET_NEXT_BLK(bp, next) (*((void **)(bp + WSIZE)) = next)
#define SET_PREV_BLK(bp, prev) (*((void **)(bp)) = prev)

/* Global variables: */
static char *heap_listp; /* Pointer to first block */
static void **list_heads;   /* Array of head free block pointer of explicit lists in segregated lists*/   
static void **list_tails;   /* Array of tail free block pointer of explicit lists in segregated lists*/
/* Function prototypes for internal helper routines: */
static void *coalesce(void *bp);
static void *extend_heap(size_t words);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
/* Function prototypes for heap consistency checker routines: */
static void checkblock(void *bp);
static void checkheap(bool verbose);
static void printblock(void *bp);
/*
 * Requires:
 *   None.
 *
 * Effects:
 *   Initialize the memory manager.  Returns 0 if the memory manager was
 *   successfully initialized and -1 otherwise.
 */
int log_2(size_t size)    /* This function returns value (log2(size) - log2(2 * DSIZE)) if size > 2 * DSIZE else returns 0 */     
{
    int ans = 0;
    while(size > 2 * DSIZE)    
    {
        size >>= 1;
        ans++;
    }
    return ans ;
}
int get_list_index(size_t size)   /* This function finds the list index of segregated list from the size of block */
{                                 /* Each list contains blocks from size (2^k) + 1  to (2^(k + 1)) */
    int list_index = 0;           
    if(size <= 2 * DSIZE)
    {
        list_index = 0;
    }
    else
    {
        list_index = log_2(size);
    }
    if(list_index > NO_OF_SEGREGATED_LISTS - 1)
    {
        list_index = NO_OF_SEGREGATED_LISTS - 1;
    }
    return list_index;
}
void add_free_blk(void *bp)                         /* This function adds free block in explicit list corresponding to its size */
{                                                                                                                           
    size_t size = GET_SIZE(HDRP(bp));             
    int list_index  = get_list_index(size);         /* Find appropriate list index for the block */
    void *curr = list_heads[list_index];                      
    if(curr == NULL)
    {                                               /* If list is empty make the block head and tail of list */
        list_heads[list_index] = bp;
        SET_NEXT_BLK(bp, NULL);
        SET_PREV_BLK(bp, NULL);
        list_tails[list_index] = bp;      
        return;
    }
    void *prev = list_tails[list_index];  /* Add the block to the end of the list and update all the links */
    SET_NEXT_BLK(prev, bp);
    SET_PREV_BLK(bp, prev);
    SET_NEXT_BLK(bp, NULL);
    list_tails[list_index] = bp;          /* Set block as the tail of the list */
    return ;
}
void remove_free_blk(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));               
    int list_index  = get_list_index(size);
    void *prev = (void *)PREV_LIST_BLKP(bp);
    void *next = (void *)NEXT_LIST_BLKP(bp);        /* Find previous and next blocks in explicit free list */
    if(prev == NULL)
    {
        list_heads[list_index] = next;    /* Remove links of bp pointer block and create links between previous
                                                        * and next blocks */
    }                                                 
    else
    {
        SET_NEXT_BLK(prev, next);
    }
    if(next == NULL)
    {
        list_tails[list_index] = prev;
    }
    else
    {
        SET_PREV_BLK(next, prev);
    }
}
int
mm_init(void)
{

    /* Create the initial empty heap. */
    if ((heap_listp = mem_sbrk(NO_OF_SEGREGATED_LISTS * DSIZE)) == (void *)-1)
        return (-1);
    list_heads = (void **)(heap_listp);     /* Initial part of heap list is array of head pointers of free blocks of explicit list */
    if ((heap_listp = mem_sbrk(NO_OF_SEGREGATED_LISTS * DSIZE)) == (void *)-1)
        return (-1);
    list_tails = (void **)(heap_listp); /* Next part of heap list is array of head pointers of free blocks of explicit list */
    int i;
    for(i = 0; i < NO_OF_SEGREGATED_LISTS; i++)
    {
        list_heads[i] = NULL;     /* Initially all pointers are set to NULL */
        list_tails[i] = NULL;
    }
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return (-1);
    PUT(heap_listp, 0);                            /* Alignment padding */
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     /* Epilogue header */
    heap_listp += (2 * WSIZE);
    /* Extend the empty heap with a free block of CHUNKSIZE bytes. */
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return (-1);
    return (0);
}

/*
 * Requires:
 *   None.
 *
 * Effects:
 *   Allocate a block with at least "size" bytes of payload, unless "size" is
 *   zero.  Returns the address of this block if the allocation was successful
 *   and NULL otherwise.
 */
void *
mm_malloc(size_t size)
{
    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    void *bp;

    /* Ignore spurious requests. */
    if (size == 0)
        return (NULL);

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE);

    /* Search the free list for a fit. */
    if ((bp = find_fit(asize)) != NULL) {   /* find_fit uses first fit algorithm */
        place(bp, asize);
        return (bp);
    }

    /* No fit found.  Get more memory and place the block. */
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return (NULL);
    place(bp, asize);
    //checkheap(0);
    return (bp);
}

/*
 * Requires:
 *   "bp" is either the address of an allocated block or NULL.
 *
 * Effects:
 *   Free a block.
 */
void
mm_free(void *bp)
{
    size_t size;

    /* Ignore spurious requests. */
    if (bp == NULL)
        return;

    /* Free and coalesce the block. */
    size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * Requires:
 *   "ptr" is either the address of an allocated block or NULL.
 *
 * Effects:
 *   Reallocates the block "ptr" to a block with at least "size" bytes of
 *   payload, unless "size" is zero.  If "size" is zero, frees the block
 *   "ptr" and returns NULL.  If the block "ptr" is already a block with at
 *   least "size" bytes of payload, then "ptr" may optionally be returned.
 *   Otherwise, a new block is allocated and the contents of the old block
 *   "ptr" are copied to that new block.  Returns the address of this new
 *   block if the allocation was successful and NULL otherwise.
 */
void *
mm_realloc(void *ptr, size_t size)       
{
    size_t oldsize;
    void *newptr;

    /* If size == 0 then this is just free, and we return NULL. */
    if (size == 0) {
        mm_free(ptr);
        return (NULL);
    }
    /* If oldptr is NULL, then this is just malloc. */
    if (ptr == NULL)
        return (mm_malloc(size));
    
    oldsize = GET_SIZE(HDRP(ptr));
    /* If size of new block required is <= oldsize then no need to find free block at other position */
    if(size + DSIZE <= oldsize)
    {
        return ptr;
    }
    /* If the block next to current ptr is free and the total size of current block and next block >= size of new required block
     *  then also no need to find free block elsewhere */
    if(!GET_ALLOC(HDRP(NEXT_BLKP(ptr))))
    {
        size_t next_blk_size = GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        if(size + DSIZE <= next_blk_size + oldsize)
        {
            remove_free_blk(NEXT_BLKP(ptr));
            PUT(HDRP(ptr),PACK(next_blk_size + oldsize, 1));
            PUT(FTRP(ptr),PACK(next_blk_size + oldsize, 1));
            return ptr;
        }
    }
    /* If above to cases fails then use mm_malloc to find the block of requires size */ 
    newptr = mm_malloc(size);

    /* If realloc() fails the original block is left untouched  */
    if (newptr == NULL)
        return (NULL);
    //checkheap(0);
    /* Copy the old data. */
    if (size < oldsize)
        oldsize = size;
    memcpy(newptr, ptr, oldsize);

    /* Free the old block. */
    mm_free(ptr);

    return (newptr);
}

/*
 * The following routines are internal helper routines.
 */

/*
 * Requires:
 *   "bp" is the address of a newly freed block.
 *
 * Effects:
 *   Perform boundary tag coalescing.  Returns the address of the coalesced
 *   block.
 */
static void *
coalesce(void *bp)
{
    bool prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    bool next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {                 /* Case 1 */
        add_free_blk(bp);                           /* Just add free block to free list (no coalesce)*/
        return (bp);
    } else if (prev_alloc && !next_alloc) {         /* Case 2 */
        remove_free_blk(NEXT_BLKP(bp));             /* Remove next block from free list */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) {         /* Case 3 */
        remove_free_blk(PREV_BLKP(bp));             /* Remove previous block from free list */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));      
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    } else {                                        /* Case 4 */
        remove_free_blk(PREV_BLKP(bp));             /* Remove previous block from free list */
        remove_free_blk(NEXT_BLKP(bp));             /* Remove next block from free list */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
            GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    add_free_blk(bp);                               /* add coalesced block to free list */
    return (bp);
}

/*
 * Requires:
 *   None.
 *
 * Effects:
 *   Extend the heap with a free block and return that block's address.
 */
static void *
extend_heap(size_t words)
{
    void *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment. */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((bp = mem_sbrk(size)) == (void *)-1)
        return (NULL);

    /* Initialize free block header/footer and the epilogue header. */
    PUT(HDRP(bp), PACK(size, 0));         /* Free block header */
    PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */

    /* Coalesce if the previous block was free. */
    return (coalesce(bp));
}

/*
 * Requires:
 *   None.
 *
 * Effects:
 *   Find a fit for a block with "asize" bytes.  Returns that block's address
 *   or NULL if no suitable block was found.
 *   First fit algorithm is used
 */
static void *
find_fit(size_t asize)
{
    void *bp = NULL;
    int list_index = get_list_index(asize);  /* Find the explicit free list corresponding to size = asize */ 
    void *it;
    bool found = false;
    while(list_index < NO_OF_SEGREGATED_LISTS)  /* Find all the lists corresponding to size >= asize till free block is found */
    {
        for(it = list_heads[list_index]; it != NULL; it = NEXT_LIST_BLKP(it))
        {                                         /* Traverse whole list to find required size block */
            if(asize <= GET_SIZE(HDRP(it)))
            {
                bp = it;
                found = true;
                break;
            }
        }
        if(found) break;
        list_index++;
    }
    return (bp);
}

/*
 * Requires:
 *   "bp" is the address of a free block that is at least "asize" bytes.
 *
 * Effects:
 *   Place a block of "asize" bytes at the start of the free block "bp" and
 *   split that block if the remainder would be at least the minimum block
 *   size.
 */
static void
place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));

    if ((csize - asize) >= (2 * DSIZE)) {  /* To avoid internal fragmentation split the block if its size is more than required size by 2 * DSIZE */
        remove_free_blk(bp);               /* Remove the block from free list */      
        PUT(HDRP(bp), PACK(asize, 1));     /* Update size and allocation tag */
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);                 /* New second block after splitting is made free and added to free list */
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
        add_free_blk(bp);                   
    } else {                                
        remove_free_blk(bp);               /* Remove the block from free list */
        PUT(HDRP(bp), PACK(csize, 1));     /* Update size and allocation tag */
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

/*
 * The remaining routines are heap consistency checker routines.
 */

/*
 * Requires:
 *   "bp" is the address of a block.
 *
 * Effects:
 *   Perform a minimal check on the block "bp".
 */
static void
checkblock(void *bp)
{

    if ((uintptr_t)bp % DSIZE)
        printf("Error: %p is not doubleword aligned\n", bp);
    if (GET(HDRP(bp)) != GET(FTRP(bp)))
        printf("Error: header does not match footer\n");
}

/*
 * Requires:
 *   None.
 *
 * Effects:
 *   Perform a minimal check of the heap for consistency.
 */
void
checkheap(bool verbose)
{
    void *bp;

    if (verbose)
        printf("Heap (%p):\n", heap_listp);

    if (GET_SIZE(HDRP(heap_listp)) != DSIZE ||
        !GET_ALLOC(HDRP(heap_listp)))
        printf("Bad prologue header\n");
    checkblock(heap_listp);

    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (verbose)
            printblock(bp);
        checkblock(bp);
    }

    if (verbose)
        printblock(bp);
    if (GET_SIZE(HDRP(bp)) != 0 || !GET_ALLOC(HDRP(bp)))
        printf("Bad epilogue header\n");
    /* Go through whole heap and for all free block find if it lies in explicit free list corresponding to its size 
     * and for all allocated blocks check that it is not present in free list */
    /* This checking is done after every 200 operations */
    bool consistent = true;                      
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) 
    {            
        size_t size = GET_SIZE(HDRP(bp));
        int list_index = get_list_index(size);
        void *it;
        bool found = false;
        for(it = list_heads[list_index]; it != NULL; it = NEXT_LIST_BLKP(it))
        {
            if(it == bp)
            {
                found = true;
                break;
            }
        }
        if(!GET_ALLOC(HDRP(bp)) && !found)
        {
            printf("Free block in heap but not found in free list: ");
            printblock(bp);
            consistent = false;
        }
        else if(GET_ALLOC(HDRP(bp)) && found)
        {
            printf("Allocated block in heap but also found in free list: ");
            printblock(bp);
            consistent = false;
        }    
    }
    if(consistent) printf("Heap and segregated free lists are consistent\n");
    bool coalesce = true;
    for(bp = heap_listp; GET_SIZE(HDRP(NEXT_BLKP(bp))) > 0; bp = NEXT_BLKP(bp))
    {
    	void *next = NEXT_BLKP(bp);
    	if(!GET_ALLOC(HDRP(bp)) && !GET_ALLOC(HDRP(next)))
    	{
    		printf("Not coalesced properly!");
    		printblock(bp);
    		coalesce = false;
    		printblock(next);
    	}	
    }
    if(coalesce) printf("Coalesced properly\n");
}

/*
 * Requires:
 *   "bp" is the address of a block.
 *
 * Effects:
 *      Print the block "bp".
 */
static void
printblock(void *bp)
{
    bool halloc, falloc;
    size_t hsize, fsize;

    checkheap(false);
    hsize = GET_SIZE(HDRP(bp));
    halloc = GET_ALLOC(HDRP(bp));
    fsize = GET_SIZE(FTRP(bp));
    falloc = GET_ALLOC(FTRP(bp));

    if (hsize == 0) {
        printf("%p: end of heap\n", bp);
        return;
    }

    printf("%p: header: [%zu:%c] footer: [%zu:%c]\n", bp,
        hsize, (halloc ? 'a' : 'f'),
        fsize, (falloc ? 'a' : 'f'));
}

/*
 * The last lines of this file configures the behavior of the "Tab" key in
 * emacs.  Emacs has a rudimentary understanding of C syntax and style.  In
 * particular, depressing the "Tab" key once at the start of a new line will
 * insert as many tabs and/or spaces as are needed for proper indentation.
 */

/* Local Variables: */
/* mode: c */
/* c-default-style: "bsd" */
/* c-basic-offset: 8 */
/* c-continued-statement-offset: 4 */
/* indent-tabs-mode: t */
/* End: */
