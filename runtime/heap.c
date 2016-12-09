/*
 * SurgeScript
 * A lightweight programming language for computer games and interactive apps
 * Copyright (C) 2016  Alexandre Martins <alemartf(at)gmail(dot)com>
 *
 * runtime/heap.c
 * SurgeScript heap
 */

#include "heap.h"
#include "variable.h"
#include "../util/util.h"

/* constants */
static const size_t SSHEAP_INITIAL_SIZE = 16;

/* the heap structure */
struct surgescript_heap_t
{
    size_t size;                /* size of the heap */
    surgescript_heapptr_t ptr;  /* allocation pointer */
    surgescript_var_t** mem;    /* data memory */
};


/* -------------------------------
 * public methods
 * ------------------------------- */

/*
 * surgescript_heap_create()
 * Creates a new heap
 */
surgescript_heap_t* surgescript_heap_create()
{
    surgescript_heap_t* heap = ssmalloc(sizeof *heap);
    size_t size = SSHEAP_INITIAL_SIZE;

    heap->mem = ssmalloc(size * sizeof(*(heap->mem)));
    heap->size = size;
    heap->ptr = size;
    while(heap->ptr)
        heap->mem[--heap->ptr] = NULL;

    return heap;
}

/*
 * surgescript_heap_destroy()
 * Destroys an existing heap
 */
surgescript_heap_t* surgescript_heap_destroy(surgescript_heap_t* heap)
{
    for(heap->ptr = 0; heap->ptr < heap->size; heap->ptr++) {
        if(heap->mem[heap->ptr] != NULL)
            surgescript_var_destroy(heap->mem[heap->ptr]);
    }

    ssfree(heap->mem);
    return ssfree(heap);
}

/*
 * surgescript_heap_malloc()
 * Allocates a memory cell
 */
surgescript_heapptr_t surgescript_heap_malloc(surgescript_heap_t* heap)
{
    for(; heap->ptr < heap->size; heap->ptr++) {
        if(heap->mem[heap->ptr] == NULL) {
            heap->mem[heap->ptr] = surgescript_var_create();
            return heap->ptr;
        }
    }

    sslog("surgescript_heap_malloc(): resizing the heap to %d cells.", heap->size * 2);
    heap->mem = ssrealloc(heap->mem, (heap->size * 2) * sizeof(*(heap->mem)));
    while(heap->ptr)
        heap->mem[heap->size + --(heap->ptr)] = NULL;
    heap->size *= 2;
    return surgescript_heap_malloc(heap);
}

/*
 * surgescript_heap_free()
 * Deallocates the memory cell pointed by ptr
 */
surgescript_heapptr_t surgescript_heap_free(surgescript_heap_t* heap, surgescript_heapptr_t ptr)
{
    if(ptr >= 0 && ptr < heap->size && heap->mem[ptr] != NULL)
        heap->mem[ptr] = surgescript_var_destroy(heap->mem[ptr]);

    return 0;
}

/*
 * surgescript_heap_at()
 * Returns the memory cell pointed by ptr
 */
surgescript_var_t* surgescript_heap_at(surgescript_heap_t* heap, surgescript_heapptr_t ptr)
{
    if(ptr >= 0 && ptr < heap->size) {
        if(heap->mem[ptr] == NULL)
            ssfatal("surgescript_heap_at(0x%x): null pointer exception.", ptr);
        return heap->mem[ptr];
    }
    else
        return NULL;
}
