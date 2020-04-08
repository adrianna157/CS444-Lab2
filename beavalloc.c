// R. Jesse Chaney
// jesse.chaney@osucascades.edu

#include "beavalloc.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>


void reset(void);

typedef struct mem_block_s {
    uint8_t free;

    size_t capacity;
    size_t size;

    struct mem_block_s *prev;
    struct mem_block_s *next;
} mem_block_t;

#define BLOCK_SIZE (sizeof(mem_block_t))
#define BLOCK_DATA(__curr) (((void *) __curr) + (BLOCK_SIZE))

static mem_block_t *block_list_head = NULL;

static void *lower_mem_bound = NULL;
static void *upper_mem_bound = NULL;

static uint8_t isVerbose = FALSE;
static FILE *beavalloc_log_stream = NULL;

// This is some gcc magic.
static void init_streams(void) __attribute__((constructor));

void *low_water = 0x0;

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

void *
beavalloc(size_t size)
{
    void *ptr = NULL;

    // your code goes here

    return ptr;
}

void 
beavfree(void *ptr)
{

    // your code goes here

    return;
}


void 
beavalloc_reset(void)
{
    //sets highwater back to the very bottom where low_water mark is, which is 0
    brk(low_water);
    
}

void *
beavcalloc(size_t nmemb, size_t size)
{
    void *ptr = NULL;


    // your code goes here

    return ptr;
}

void *
beavrealloc(void *ptr, size_t size)
{
    void *nptr = NULL;

    // your code goes here

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
