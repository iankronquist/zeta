#ifndef __VM_H__
#define __VM_H__

#include <stdlib.h>
#include <stdint.h>

/// Heap object pointer
typedef uint8_t* heapptr_t;

/// Heap object descriptor/header
typedef uint64_t desc_t;

/// Heap object descriptors
#define DESC_STRING     1
#define DESC_ARRAY      2
#define DESC_AST_INT    3
#define DESC_AST_BINOP  4
#define DESC_AST_UNOP   5
#define DESC_AST_IF     6
#define DESC_AST_CALL   7

/// Initial VM heap size
#define HEAP_SIZE (1 << 24)

/**
Word value type
*/
typedef union
{
    int64_t int64;

    heapptr_t heapptr;

} word_t;

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
    desc_t desc;

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
    desc_t desc;

    /// Allocated capacity
    uint32_t cap;

    /// Array length
    uint32_t len;

    /// Array elements, variable length
    word_t elems[];

} array_t;

/// Initialize the global VM instance
void vm_init();

heapptr_t vm_alloc(uint32_t size, desc_t desc);

string_t* string_alloc(uint32_t len);

array_t* array_alloc(uint32_t cap);
void array_set(array_t* array, uint32_t idx, word_t val);
void array_set_ptr(array_t* array, uint32_t idx, heapptr_t val);
word_t array_get(array_t* array, uint32_t idx);

#endif

