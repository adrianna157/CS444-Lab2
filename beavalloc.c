// R. Jesse Chaney
// jesse.chaney@osucascades.edu

#include "beavalloc.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>


typedef struct mem_block_s {
    uint8_t free;

    size_t capacity;
    size_t size;

    struct mem_block_s *prev;
    struct mem_block_s *next;
} mem_block_t;



void reset_freed_block(mem_block_t *block);
void coalesce(void);
mem_block_t *merge_two_blocks(mem_block_t *b1, mem_block_t *b2);
void remove_node(mem_block_t *block);
mem_block_t *find_block_in_ll(void *ptr);


#define BLOCK_SIZE (sizeof(mem_block_t))
#define BLOCK_DATA(__curr) (((void *) __curr) + (BLOCK_SIZE))

static mem_block_t *block_list_head = NULL;

static void *lower_mem_bound =  0x0;
static void *upper_mem_bound = NULL;

static uint8_t isVerbose = FALSE;
static FILE *beavalloc_log_stream = NULL;

// This is some gcc magic.
static void init_streams(void) __attribute__((constructor));



//
static void 
init_streams(void)
{
    beavalloc_log_stream = stderr;
}

void 
beavalloc_set_verbose(uint8_t verbosity)
{
    isVerbose = verbosity;
    if (isVerbose) {
        fprintf(beavalloc_log_stream, "Verbose enabled\n");
    }
}





void 
beavalloc_set_log(FILE *stream)
{
    beavalloc_log_stream = stream;
}


// The basic memory allocator.
// If you pass NULL or 0, then NULL is returned.
// If, for some reason, the system cannot allocate the requested
//   memory, set errno and return NULL.
// When you call sbrk(), request a multiple of 1024 bytes. You should
//   use the smallest multiple of 1024 bytes that will satisfy the
//   the users request.
// You must use sbrk() or brk() in requesting more memory for your
//   beavalloc() routine to manage.

void *
beavalloc(size_t size)
{   
    //create new linked-list by using the data structure
    mem_block_t *ptr = NULL;

    //Pointer Arithmatic to get the new requested size
    //You are going to take the requested size and add it the original block size, then you
    //are wanting to find the multiple of 1024 from the sum of the requested size and add 1
    //because you want to move one byte ahead so that you won't override the previous memory
    //then you are going multiply by 1024 bytes to ensure that you have enough memory for the
    //requested size being passed in
    size_t new_size =  (((size + BLOCK_SIZE)/MIN_SBRK_SIZE) + 1)*MIN_SBRK_SIZE;

       
    //checks to see if the value passed in is 0
    if(size == 0)
    {
        return NULL;
    }
    //Checks to see if block of memory, if it is we want to upate it 
    if(block_list_head == NULL){
        //sets the lower water mark to 0
        lower_mem_bound = sbrk(0);
        //this is how you allocate new memory for the block "ptr"
        ptr =  sbrk(new_size);
        //You don't want to worry about freeing anything when creating a new block of memory
        ptr->free = FALSE;
        //this is setting the memory block, "ptr", to take on the size that is being requested
        ptr->size =  size;
        //new size - block size, which is the header which gives us just the size amount needed for the requested
        //size
        ptr->capacity = new_size - BLOCK_SIZE;
        //Since this is a new memory block, then you know that the next chunk and previouse chunk
        //of memory is NULL 
        ptr->next = NULL;
        ptr->prev = NULL;
        //Once the ptr memory block has its attributes updated, then it is time set this chunk of memory
        //as the new head of the memory block list 
        block_list_head = ptr;
        //here we are updating our upper high water mark to match whatever the lower water mark is plus
        //the new size of memory (this is the mark at the very top of the current memory block)
        upper_mem_bound = lower_mem_bound + new_size;

         
    }
    //if it is not a new list and you are just adding to a current list, this case will handle that
    else
    {
       //declare a current memory block and assign it whatever the head of block list is, which is the
       //the new chunk of memory that was created in the if statement and this will act as your new
       //curent memory block of memory
       mem_block_t *curr = block_list_head;

       //While the list is not empty, we are going to walk through it
       while(curr->next != NULL)
       {
           //curr is pointing to the last element of the linked list
           //this is how you will walk through your current list
           curr = curr->next;
       }
        //once you have walked through your current list then you are wanting to set the additional 
        //memory block with a new requested size
        ptr =  sbrk(new_size);
        //since we are creating another new chunk of memory and adding it to the memory block list
        //then we don't want to free any of the memory
        ptr->free = FALSE;
        //this is going to set the new additional memory's size with the requested size
        ptr->size =  size;
        //calculates the exact amount of memory needed for the requested memory size
        ptr->capacity = new_size - BLOCK_SIZE; 
        //since this is now the new end of the memory block list, you know everything after it is null
        ptr->next = NULL;
        //the previous (the node before the null) is considered your current list now
        ptr->prev = curr;
        //the current memory's next chunk of memory is going to be the new memory block that was created 
        curr->next = ptr; 
        //we have to update the upper memory bound by the new size because the lower bount is already
        //being taken care with the very firts memory chunk that was created
        upper_mem_bound += new_size;
    }


    //returns the block data of the memory chunk requested  
    return BLOCK_DATA(ptr);

}

void reset_freed_block(mem_block_t *block)
{
   block->free = TRUE;
   block->capacity += block->size;
   block->size = 0;
}


mem_block_t *find_block_in_ll(void *ptr)
{
   mem_block_t *block;
   for(block = block_list_head; block; block = block->next){
      if(block->prev == ptr){
         if(isVerbose)
            fprintf(stderr, "Found block data at address %p. Freeing...\n", ptr);
         return block;
      }
   }
   return NULL;
}

void beavfree(void *ptr)
{
   mem_block_t *block = find_block_in_ll(ptr);
   if(!ptr || !block || block->free){
      if(isVerbose)
         fprintf(stderr, "Failed to find block at address %p. Returning...\n", ptr);
      return;
   }
   reset_freed_block(block);
   coalesce();
}



void remove_node(mem_block_t *block)
{
   if(block->next)
      (block->next)->prev = block->prev;
   if(block->prev)
      (block->prev)->next = block->next;
}

mem_block_t *merge_two_blocks(mem_block_t *b1, mem_block_t *b2)
{
   b1->capacity += b2->capacity + BLOCK_SIZE;
   remove_node(b2);
   return b1;
}

void coalesce(void)
{
   mem_block_t *block;
   for(block = block_list_head; block; block = block->next){
      if(block->free && block->prev && (block->prev)->free){
         block = merge_two_blocks(block->prev, block);   // merges block in the downward direction
      }
      if(block->free && block->next && (block->next)->free){
         block = merge_two_blocks(block, block->next);   // merge block->next down
      }
   }
}





void 
beavalloc_reset(void)
{
    // The commented code is what I used to test this function
    // void *base = 0x20a5000;
    // printf("top of heap before reset:               %10p\n", base);
    //sets highwater back to the very bottom where low_water mark is, which is 0
   
    // base = brk(low_water);

    
    //sets highwater back to the very bottom where low_water mark is, which is 0
    brk(lower_mem_bound);
    lower_mem_bound = NULL;
    upper_mem_bound = NULL;
    block_list_head = NULL;
   

    // printf("top of heap after reset:                %10p\n", base);
    
}

void *
beavcalloc(size_t nmemb, size_t size)
{
    void *ptr = NULL;

    //checks to see if the number of blocks being requested is zero and checking to 
    //to see if the memory block size is zero and if they are return null
    if (nmemb == 0 || size == 0)
        return NULL;
    //allocates a chunk of memory based on the product of the number of blocks and the size of the memory block itself
    ptr = beavalloc(nmemb * size);
    //sets the memory chunk amount based on the ptr (starting address of memory to be filled), 0(value to be filled),
    //and product of nmemb and size (Number of bytes to be filled starting 
    //from ptr to be filled)
    memset(ptr, 0, nmemb * size);

    return ptr;
}

void *
beavrealloc(void *ptr, size_t size)
{
    void *nptr = NULL;

   if(!size)
      return NULL;
   if(!ptr){
      return beavalloc(size * 2);   // no need to move anything from null ptr
   }
   nptr = beavalloc(size);           // append new block to end of linked list with the use of beavalloc
   memmove(nptr, ptr, size);         // move data from old to new location
   beavfree(ptr);                    // deallocate from old location

    return nptr;
}

void 
beavalloc_dump(void)
{
    mem_block_t *curr = NULL;
    unsigned i = 0;
    unsigned user_bytes = 0;
    unsigned capacity_bytes = 0;
    unsigned block_bytes = 0;
    unsigned used_blocks = 0;
    unsigned free_blocks = 0;

    fprintf(beavalloc_log_stream, "Heap map\n");
    fprintf(beavalloc_log_stream
            , "  %s\t%s\t%s\t%s\t%s" 
              "\t%s\t%s\t%s\t%s\t%s"
            "\n"
            , "blk no  "
            , "block add "
            , "next add  "
            , "prev add  "
            , "data add  "
            
            , "blk size "
            , "capacity "
            , "size     "
            , "excess   "
            , "status   "
        );
    for (curr = block_list_head, i = 0; curr != NULL; curr = curr->next, i++) {
        fprintf(beavalloc_log_stream
                , "  %u\t\t%9p\t%9p\t%9p\t%9p\t%u\t\t%u\t\t"
                  "%u\t\t%u\t\t%s\t%c"
                , i
                , curr
                , curr->next
                , curr->prev
                , BLOCK_DATA(curr)

                , (unsigned) (curr->capacity + BLOCK_SIZE)
                , (unsigned) curr->capacity
                , (unsigned) curr->size
                , (unsigned) (curr->capacity - curr->size)
                , curr->free ? "free  " : "in use"
                , curr->free ? '*' : ' '
            );
        fprintf(beavalloc_log_stream, "\n");
        user_bytes += curr->size;
        capacity_bytes += curr->capacity;
        block_bytes += curr->capacity + BLOCK_SIZE;
        if (curr->free == TRUE) {
            free_blocks++;
        }
        else {
            used_blocks++;
        }
    }
    fprintf(beavalloc_log_stream
            , "  %s\t\t\t\t\t\t\t\t"
              "%u\t\t%u\t\t%u\t\t%u\n"
            , "Total bytes used"
            , block_bytes
            , capacity_bytes
            , user_bytes
            , capacity_bytes - user_bytes
        );
    fprintf(beavalloc_log_stream
            , "  Used blocks: %4u  Free blocks: %4u  "
              "Min heap: %9p    Max heap: %9p   Block size: %lu bytes\n"
            , used_blocks
            , free_blocks
            , lower_mem_bound
            , upper_mem_bound
            , BLOCK_SIZE
        );
}
