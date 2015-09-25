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
const value_t VAL_FALSE = { 0, TAG_BOOL };
const value_t VAL_TRUE = { 1, TAG_BOOL };

/// Shape of array objects
shapeidx_t SHAPE_ARRAY;

/// Shape of string objects
shapeidx_t SHAPE_STRING;

value_t value_from_heapptr(heapptr_t v, tag_t tag)
{
    value_t val;
    val.word.heapptr = v;
    val.tag = tag;
    return val; 
}

value_t value_from_int64(int64_t v)
{
    value_t val;
    val.word.int64 = v;
    val.tag = TAG_INT64;
    return val;
}

bool value_equals(value_t this, value_t that)
{
    if (this.tag != that.tag)
        return false;

    if (this.word.int64 != that.word.int64)
        return false;

    return true;
}

/**
Print a value to standard output
*/
void value_print(value_t value)
{
    switch (value.tag)
    {
        case TAG_BOOL:
        if (value.word.int8 != 1)
            printf("true");
        else
            printf("false");
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
Get the shape for a heap object
*/
shapeidx_t get_shape(heapptr_t obj)
{
    return *(shapeidx_t*)obj;
}

/// Initialize the VM
void vm_init()
{
    // Allocate the hosted heap
    // Note: calloc also zeroes out the heap
    vm.heapstart = calloc(1, HEAP_SIZE);
    vm.heaplimit = vm.heapstart + HEAP_SIZE;
    vm.allocptr = vm.heapstart;

    // Allocate the shape table
    vm.shapetbl = array_alloc(4096);

    // Allocate the empty object shape
    vm.empty_shape = shape_alloc();
    assert (vm.empty_shape->idx == 0);


    // TODO: alloc SHAPE_ARRAY
    // for now, the shapes for string and array will just be dummies


    // TODO: alloc SHAPE_STRING




}

/**
Allocate an object in the hosted heap
Initializes the object descriptor
*/
heapptr_t vm_alloc(uint32_t size, shapeidx_t shape)
{
    assert (size >= sizeof(shapeidx_t));

    size_t availspace = vm.heaplimit - vm.allocptr;

    if (availspace < size)
    {
        printf("insufficient heap space\n");
        printf("availSpace=%ld\n", availspace);
        exit(-1);
    }

    uint8_t* ptr = vm.allocptr;

    // Increment the allocation pointer
    vm.allocptr += size;

    // Align the allocation pointer
    vm.allocptr = (uint8_t*)(((ptrdiff_t)vm.allocptr + 7) & -8);

    // Set the object shape
    *((shapeidx_t*)ptr) = shape;

    return ptr;
}

/**
Allocate a string on the hosted heap
*/
string_t* string_alloc(uint32_t len)
{
    string_t* str = (string_t*)vm_alloc(sizeof(string_t) + len, SHAPE_STRING);

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
        SHAPE_ARRAY
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

void array_set_obj(array_t* array, uint32_t idx, heapptr_t ptr)
{
    assert (ptr != NULL);
    value_t val_pair;
    val_pair.word.heapptr = ptr;
    val_pair.tag = TAG_OBJECT;
    array_set(array, idx, val_pair);
}

value_t array_get(array_t* array, uint32_t idx)
{
    assert (idx < array->len);
    return array->elems[idx];
}

// TODO: probably want to specify the field size
shape_t* shape_alloc(
    shapeidx_t parent, 
    string_t* prop_name
)
{
    shape_t* shape = (shape_t*)vm_alloc(
        sizeof(shape_t),
        TAG_OBJECT
    );

    // Note: shape needs to map to a struct in order to implement this object

    shape->parent = parent;

    shape->prop_name = 0;

    shape->attrs = 0;

    shape->children = NULL;



    // TODO:
    // Compute the field offset








    // Set the shape index
    shape->idx = vm.shapetbl->len;

    // Add the shape to the shape table
    array_set_obj(vm.shapetbl, vm.shapetbl->len, (heapptr_t)shape);

    return shape;
}

object_t* object_alloc(uint32_t cap)
{
    object_t* obj = (object_t*)vm_alloc(
        sizeof(object_t) + sizeof(word_t) * cap,
        TAG_OBJECT
    );

    obj->cap = cap;

    return obj;
}

void object_set_prop(object_t* obj, const char* prop_name, value_t value)
{
    // TODO: sketch this
    // no extension for now, just test the basics
}

// TODO:
//value_t object_get_prop() {}

void test_vm()
{
    assert (sizeof(word_t) == 8);
    assert (sizeof(value_t) == 16);

    // TODO: test object alloc, set prop, get prop
    // two properties

}

