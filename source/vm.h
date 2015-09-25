#ifndef __VM_H__
#define __VM_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/// Heap object pointer
typedef uint8_t* heapptr_t;

/// Value tag
typedef uint8_t tag_t;

/// Shape index (object header)
typedef uint32_t shapeidx_t;

/// Value type tags
#define TAG_BOOL        0
#define TAG_INT64       1
#define TAG_FLOAT64     2
#define TAG_STRING      3
#define TAG_ARRAY       4
#define TAG_RAW_PTR     5
#define TAG_OBJECT      6
#define TAG_CLOS        7

/// Initial VM heap size
#define HEAP_SIZE (1 << 24)

// Forward declarations
typedef struct array array_t;
typedef struct string string_t;
typedef struct shape shape_t;
typedef struct object object_t;

/**
Word value type
*/
typedef union
{
    int8_t int8;
    int16_t int16;
    int32_t int32;
    int64_t int64;
    double float64;

    heapptr_t heapptr;

    array_t* array;
    string_t* string;
    object_t* object;

    tag_t tag;

} word_t;

/*
Tagged value pair type
*/
typedef struct
{
    word_t word;

    tag_t tag;

} value_t;

/// Boolean constant values
const value_t VAL_FALSE;
const value_t VAL_TRUE;

/**
Virtual machine
*/
typedef struct
{
    uint8_t* heapstart;

    uint8_t* heaplimit;

    uint8_t* allocptr;

    /// Global object
    object_t* global;

    array_t* shapetbl;

    array_t* stringtbl;

} vm_t;

/**
String (heap object)
*/
typedef struct string
{
    tag_t tag;

    /// String hash
    uint32_t hash;

    /// String length
    uint32_t len;

    /// Character data, variable length
    char data[];

} string_t;

/**
Array (list) heap object
*/
typedef struct array
{
    tag_t tag;

    /// Allocated capacity
    uint32_t cap;

    /// Array length
    uint32_t len;

    /// Array elements, variable length
    /// Note: each value is tagged
    value_t elems[];

} array_t;

/// Type tag known property attribute
#define ATTR_TAG_KNOWN  (1 << 0)

/// Property word known attribute
#define ATTR_WORD_KNOWN (1 << 1)

/// Read-only property attribute
#define ATTR_READ_ONLY  (1 << 2)

/// Object frozen attribute
/// Frozen means shape cannot change, read-only and no new properties
#define ATTR_OBJ_FROZEN (1 << 3)

/*
Shape node descriptor
Property tags should immediately follow properties, if unknown
*/
typedef struct shape
{
    tag_t tag;

    /// Index of this shape node
    tag_t shape_idx;

    /// Property name
    string_t* prop_name;

    /// Parent shape index
    tag_t parent_idx;

    /// Slot index
    uint32_t slot_idx;

    /// Property word/value, if known
    value_t prop_val;

    /// Property and object attributes
    uint8_t attrs;

    // TODO: child shapes
    // KISS for now, just an array

} shape_t;

/**
Object
*/
typedef struct object
{
    tag_t tag;

    /// Capacity in bytes
    uint32_t cap;

    /// Object extension table
    object_t* ext_tbl;

} object_t;

value_t value_from_heapptr(heapptr_t v);
value_t value_from_int64(int64_t v);
void value_print(value_t value);
bool value_equals(value_t this, value_t that);

tag_t get_tag(heapptr_t obj);

void vm_init();
heapptr_t vm_alloc(uint32_t size, shapeidx_t shape);

string_t* string_alloc(uint32_t len);
void string_print(string_t* str);

array_t* array_alloc(uint32_t cap);
void array_set(array_t* array, uint32_t idx, value_t val);
void array_set_ptr(array_t* array, uint32_t idx, heapptr_t val);
value_t array_get(array_t* array, uint32_t idx);

void test_vm();

#endif

