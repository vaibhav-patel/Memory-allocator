# Memory-allocator
Malloc: Writing a Dynamic Storage Allocator

Simple, 32-bit and 64-bit clean allocator based on a Hashmap (i.e. segregated list) for accesing list , on an average constant time 
	fit placement for block placement and boundary tag coalescing. Blocks are aligned to 16-byte boundaries. 
 	This yields 16-byte aligned blocks on a 64-bit processor and 32-bit processor.  
 	However, 16-byte alignment is stricter than necessary; the assignment only requires 8-byte alignment.  
 	The minimum block size is four words. i.e two double words. 
 
    This allocator defines the size of a pointer to 8-byte. 


My implementation of the dynamic storage allocator consists of the following fuctions, as well as some additional helper functions:



int mm_init(void): Performs necessary initializations
void *mm_malloc(size_t size): returns a pointer to the allocated block payload
void mm_free(void *ptr): frees the block pointed to by ptr
void *mm_realloc(void *ptr, size_t size): returns a pointer to the allocated bock payload
void mm_checkheap(void): debugging function, scans the heap and checks for consistency
static void *coalesce(void *bp);
static void *extend_heap(size_t words);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
static void checkblock(void *bp);
static void printblock(void *bp);
int log_2(size_t size) : returns the log2 of the given size minus log2(2 * DSIZE) for size >= 2 * DSIZE else 0
int get_list_index(size_t size): gets the list index defined according to block size.
void add_free_blk(void *bp): adds free block to the hashmap in its appropiate index using get_list_index(size_t size) using LIFO algo in constant order of time.
void remove_free_blk(void *bp) :removes free block from the hashmap using the given ptr and its index.removing is also takes O(1) time.


Each block has header and footer of the form:
 
      	x	              3  2  1  0
       -----------------------------------
      | s  s  s  s  ... s  s  s  0  0  1/0| 
       -----------------------------------
       where x = 2 * DSIZE - 1
 where s are the meaningful size bits and a/f is set
 if the block is allocated. The list has the following form:
 
    begin                     									    end
                                                            heap_listp								  
    ---------------------------------------------------------------------------------------------------
    | | | | | | | | | | | | | | | | |  header(8|1)||header(8|1)| MEMORY BLOCKS            |header(0|1)|
    ---------------------------------------------------------------------------------------------------
    |     segregate list  | pading    | prologue  || prologue  |        	          |  epilogue |
    |     heads 		                                                          |     block |

  The allocated prologue and epilogue blocks are overhead that
  eliminate edge conditions during coalescing.
 
  Segregated lists: partition the block sizes by powers of two 

  My implementation uses a doubly linked hashmap(i.e. segregated list) for memory management and a first fit scanning algorithm for allocating memory.


 
  MALLOCING:
  In our malloc funtion, a block is allocated by looking through all elements in the
  free list instead of traversing all the blocks. In free list we have NO_OF_SEGREGATED_LIST different pointers 
  to maintain the doubly linked list header and footer. Then we delete the first free block 
  which has a size >= the requested size given to malloc. If a free block is big enough to 
  split into another usable block then we split it using place function. Then we delete the block from the explicit free list 
  and then adding new splitted free block with parameters of the block whose size and allocation is updated.

  FREEING:
  Whenever user frees a block and calls free(ptr). Then we will add the block in explicit free list 
  and change its header and footer information to include a 0 allocation bit. Newly freed block 
  is added at the beginning of the explicit free list corresponding to its size. Adding block to free list is done after 
  coalescing.
 
  REALLOCING:
  * If the new size is less than the old size, shrink the block and change the header and footer.
  * If increasing size, we first check next (not previous) block allocation status
  if it is free block and its size is big enough for our purpose then we simply combine 
  them to make bigger block and then we don't have to copy the data because it is already there.
  but if we can't get free block or it is free but doesn't meet with with the requirements 
  we have to call malloc with new size, copy old data, then free old block.
  * If size is same, keep same block.
 
  ANATOMY OF BLOCKS:
  This is how a free block looks:		HEADER | PREV FREE | NEXT FREE | OLD DATA | FOOTER
  This is how an allocated block looks:	HEADER |--------------DATA----------------| FOOTER
 
  In the header and footer: 8-byte word containing size of block and allocation flag bit

  CHECKHEAP:
  We have added a routine that checks all the free blocks in heap whether they are present in explicit list of corresponding size or not. Also we check all allocated blocks in heap to check that they are not present in explicit free list. We also check whole heap to see that blocks are coalesced properly and no two adjacent blocks are free.


 
