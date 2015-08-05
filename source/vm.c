#include "vm.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

/// Global VM instance
vm_t vm;

/// Initialize the VM
void vm_init()
{
    // Allocate the hosted heap
    // Note: calloc also zeroes out the heap
    vm.heapStart = calloc(1, HEAP_SIZE);
    vm.heapLimit = vm.heapStart + HEAP_SIZE;
    vm.allocPtr = vm.heapStart;
}

/**
Allocate an object in the hosted heap
Initializes the object descriptor
*/
heapptr_t vm_alloc(uint32_t size, desc_t desc)
{
    assert (size >= sizeof(desc_t));

    size_t availSpace = vm.heapLimit - vm.allocPtr;

    if (availSpace < size)
    {
        printf("insufficient heap space\n");
        printf("availSpace=%ld\n", availSpace);
        exit(-1);
    }

    uint8_t* ptr = vm.allocPtr;

    // Increment the allocation pointer
    vm.allocPtr += size;

    // Align the allocation pointer
    vm.allocPtr = (uint8_t*)(((ptrdiff_t)vm.allocPtr + 7) & -8);

    // Set the object descriptor
    *((desc_t*)ptr) = desc;

    return ptr;
}

/**
Allocate a string on the hosted heap
*/
string_t* string_alloc(uint32_t len)
{
    string_t* str = (string_t*)vm_alloc(sizeof(string_t) + len, DESC_STRING);

    str->len = len;

    return str;
}

bool string_eq(string_t* stra, string_t* strb)
{
    if (stra->len != strb->len)
        return false;

    return strncmp(stra->data, strb->data, stra->len) == 0;
}

array_t* array_alloc(uint32_t cap)
{
    // Note: the heap is zeroed out on allocation
    array_t* arr = (array_t*)vm_alloc(
        sizeof(array_t) + cap * sizeof(word_t),
        DESC_ARRAY
    );

    arr->cap = cap;
    arr->len = 0;

    return arr;
}

void array_set_length(array_t* array, uint32_t len)
{
    assert (len <= array->cap);

    array->len = len;
}

void array_set(array_t* array, uint32_t idx, word_t val)
{
    if (idx >= array->len)
        array_set_length(array, idx+1);

    array->elems[idx] = val;
}

void array_set_ptr(array_t* array, uint32_t idx, heapptr_t val)
{
    word_t word;
    word.heapptr = val;
    array_set(array, idx, word);
}

word_t array_get(array_t* array, uint32_t idx)
{
    assert (idx < array->len);

    return array->elems[idx];
}

