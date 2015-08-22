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

/// Boolean constant values
const value_t VAL_FALSE = { 0, TAG_FALSE };
const value_t VAL_TRUE = { 0, TAG_TRUE };

value_t value_from_heapptr(heapptr_t v)
{
    value_t val;
    val.word.heapptr = v;
    val.tag = get_tag(v);
    return val; 
}

value_t value_from_int64(int64_t v)
{
    value_t val;
    val.word.int64 = v;
    val.tag = TAG_INT64;
    return val;
}

/**
Print a value to standard output
*/
void value_print(value_t value)
{
    switch (value.tag)
    {
        case TAG_FALSE:
        printf("false");
        break;

        case TAG_TRUE:
        printf("true");
        break;

        case TAG_INT64:
        printf("%ld", value.word.int64);
        break;

        case TAG_FLOAT64:
        printf("%lf", value.word.float64);
        break;

        case TAG_STRING:
        {
            putchar('"');
            string_print((string_t*)value.word.heapptr);
            putchar('"');
        }
        break;

        case TAG_ARRAY:
        {
            array_t* array = (array_t*)value.word.heapptr;

            putchar('[');
            for (size_t i = 0; i < array->len; ++i)
            {
                value_print(array_get(array, i));
                if (i + 1 < array->len)
                    printf(", ");
            }
            putchar(']');
        }
        break;

        default:
        printf("unknown value tag");
        break;
    }
}

/**
Get the tag for a heap object
*/
tag_t get_tag(heapptr_t obj)
{
    return *(tag_t*)obj;
}

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
heapptr_t vm_alloc(uint32_t size, tag_t tag)
{
    assert (size >= sizeof(tag_t));

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
    *((tag_t*)ptr) = tag;

    return ptr;
}

/**
Allocate a string on the hosted heap
*/
string_t* string_alloc(uint32_t len)
{
    string_t* str = (string_t*)vm_alloc(sizeof(string_t) + len, TAG_STRING);

    str->len = len;

    return str;
}

void string_print(string_t* str)
{
    for (size_t i = 0; i < str->len; ++i)
        putchar(str->data[i]);
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
        sizeof(array_t) + cap * sizeof(value_t),
        TAG_ARRAY
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

void array_set(array_t* array, uint32_t idx, value_t val)
{
    if (idx >= array->len)
        array_set_length(array, idx+1);

    array->elems[idx] = val;
}

void array_set_ptr(array_t* array, uint32_t idx, heapptr_t ptr)
{
    assert (ptr != NULL);
    value_t val_pair;
    val_pair.word.heapptr = ptr;
    val_pair.tag = get_tag(ptr);
    array_set(array, idx, val_pair);
}

value_t array_get(array_t* array, uint32_t idx)
{
    assert (idx < array->len);
    return array->elems[idx];
}

void test_vm()
{
    assert (sizeof(word_t) == 8);
    assert (sizeof(value_t) == 16);
}

