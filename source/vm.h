#ifndef __VM_H__
#define __VM_H__

#include <stdlib.h>
#include <stdint.h>

/// Heap object pointer
typedef uint8_t* heapptr_t;

/// Value tag (and object header)
typedef uint64_t tag_t;

/// Non-object value tags
/// Note: the boolean false has tag zero
#define TAG_FALSE       0
#define TAG_TRUE        1
#define TAG_INT64       2
#define TAG_FLOAT64     3
#define TAG_STRING      4
#define TAG_ARRAY       5
#define TAG_RAW_PTR     6

// FIXME: does it make sense for object to have a tag of its own?
// Probably, the empty object should have a fixed tag?
// We may want to auto-assign the following tags by building shape nodes

/// Object value tags
/// Note: object values have the least bit set to one
#define TAG_OBJECT      7
#define TAG_CLOS        8
#define TAG_AST_CONST   9
#define TAG_AST_REF     10
#define TAG_AST_BINOP   11
#define TAG_AST_UNOP    12
#define TAG_AST_SEQ     13
#define TAG_AST_IF      14
#define TAG_AST_CALL    15
#define TAG_AST_FUN     16
#define TAG_RUN_ERR     17

/// Initial VM heap size
#define HEAP_SIZE (1 << 24)

/**
Word value type
*/
typedef union
{
    int64_t int64;
    double float64;

    heapptr_t heapptr;

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
    uint8_t* heapStart;

    uint8_t* heapLimit;

    uint8_t* allocPtr;

    // TODO: global variable slots
    // use naive lookup for now, linear search
    // - big array, names and value words
    // eventually, use global object

} vm_t;

/**
String (heap object)
*/
typedef struct
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
typedef struct
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

value_t value_from_heapptr(heapptr_t v);
value_t value_from_int64(int64_t v);
void value_print(value_t value);

tag_t get_tag(heapptr_t obj);

void vm_init();
heapptr_t vm_alloc(uint32_t size, tag_t tag);

string_t* string_alloc(uint32_t len);
void string_print(string_t* str);

array_t* array_alloc(uint32_t cap);
void array_set(array_t* array, uint32_t idx, value_t val);
void array_set_ptr(array_t* array, uint32_t idx, heapptr_t val);
value_t array_get(array_t* array, uint32_t idx);

void test_vm();

#endif

