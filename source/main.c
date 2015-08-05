#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

//============================================================================
// VM and interpreter
//============================================================================

/// Heap object pointer
typedef uint8_t* heapptr_t;

/// Word value type
typedef union
{
    int64_t int64;

    heapptr_t heapptr;

} word_t;

/// Initial VM heap size
const size_t HEAP_SIZE = 1 << 24;

/**
Virtual machine
*/
typedef struct
{
    uint8_t* heapStart;

    uint8_t* heapLimit;

    uint8_t* allocPtr;

    // TODO: global variable slots?
    // use naive lookup for now, linear search





} vm_t;

/// Global VM instance
vm_t vm;

/// Initialize the VM
void vm_init()
{
    // Allocate the hosted heap
    vm.heapStart = malloc(HEAP_SIZE);
    vm.heapLimit = vm.heapStart + HEAP_SIZE;
    vm.allocPtr = vm.heapStart;
}

/// Heap object descriptor/header
typedef uint64_t desc_t;

const desc_t DESC_STRING    = 1;
const desc_t DESC_ARRAY     = 2;
const desc_t DESC_AST_INT   = 3;

/// Allocate an object in the hosted heap
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
Allocate a string on the hosted heap
*/
string_t* string_alloc(uint32_t len)
{
    string_t* str = (string_t*)vm_alloc(sizeof(string_t) + len, DESC_STRING);

    str->len = len;

    return str;
}

// TODO: string_eq
bool string_eq()
{
}

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

array_t* array_alloc(uint32_t num_elems)
{
    // TODO



}

void array_set(array_t* array, uint32_t idx, word_t val)
{
    assert (idx < array->len);

    array->elems[idx] = val;
}

word_t array_get(array_t* array, uint32_t idx)
{
    assert (idx < array->len);

    return array->elems[idx];
}



// TODO: evalExpr



// TODO: need some kind of environment object
// map variable name strings to values
// for now, no type tags





// ===========================================================================
// AST nodes
// Note: all AST nodes are heap objects
// ===========================================================================

/**
Integer node
*/
typedef struct
{
    desc_t desc;

    int64_t val;

} ast_int_t;

ast_int_t* ast_int_alloc(int64_t val)
{
    ast_int_t* node = (ast_int_t*)vm_alloc(sizeof(ast_int_t), DESC_AST_INT);
    node->val = val;
    return node;
}



// TODO: function call node
// save until string and function parsing works




/**
Function expression node
*/
typedef struct
{
    desc_t desc;

    // List of parameter names (strings)
    array_t* param_names;

    /// Function body expression
    heapptr_t body_expr;

} ast_fun_t;

//============================================================================
// Parser
//============================================================================

#include <ctype.h>
#include <string.h>

/**
Input stream, character/token stream for parsing functions
*/
typedef struct
{
    /// Internal string (hosted heap)
    string_t* str;

    /// Current index
    uint32_t idx;

    /// Current position, srcpos_t
    //add this later

} input_t;

input_t input_from_string(string_t* str)
{
    input_t input;
    input.str = str;
    input.idx = 0;
    return input;
}

/// Test if the end of file has been reached
bool input_eof(input_t* input)
{
    assert (input->str != NULL);
    return (input->idx >= input->str->len);
}

/// Peek at a character from the input
char input_peek_ch(input_t* input)
{
    assert (input->str != NULL);

    if (input->idx >= input->str->len)
        return '\0';

    return input->str->data[input->idx];
}

/// Read a character from the input
char input_read_ch(input_t* input)
{
    char ch = input_peek_ch(input);
    input->idx++;
    return ch;
}

/**
Parse an identifier
*/
heapptr_t parseIdent(input_t* input)
{
    size_t startIdx = input->idx;

    size_t len = 0;

    for (;;)
    {
        char ch = input_peek_ch(input);

        if (!isalnum(ch) && ch != '$' && ch != '_')
            break;

        // Consume this character
        input_read_ch(input);
        len++;
    }

    if (len == 0)
        return NULL;

    string_t* str = string_alloc(len);

    // Copy the characters
    strncpy(str->data, input->str->data + startIdx, len);

    return (heapptr_t)str;
}

/**
Parse an integer value
*/
heapptr_t parseInt(input_t* input)
{
    size_t numDigits = 0;
    int64_t intVal = 0;

    for (;;)
    {
        char ch = input_peek_ch(input);

        if (!isdigit(ch))
            break;

        intVal = 10 * intVal + (ch - '0');

        // Consume this digit
        input_read_ch(input);
        numDigits++;
    }

    if (numDigits == 0)
        return NULL;

    return (heapptr_t)ast_int_alloc(intVal);
}


// TODO: parseExprList


/**
Parse a call expression
ident(x y z ...)
*/
heapptr_t parseCall(input_t* input)
{
    // TODO




}

/**
Parse an expression from an input stream
Returns an IR value
*/
heapptr_t parseExpr(input_t* input)
{
    printf("idx=%d\n", input->idx);

    // Consume whitespace
    while (isspace(input_peek_ch(input)))
        input_read_ch(input);

    input_t sub;

    heapptr_t expr;

    // Try parsing an integer
    sub = *input;
    expr = parseInt(&sub);
    if (expr != NULL)
    {
        *input = sub;
        return expr;
    }

    // Try parsing an identifier
    sub = *input;
    expr = parseIdent(&sub);
    if (expr != NULL)
    {
        *input = sub;
        return expr;
    }







    // Parsing failed
    return NULL;
}

//============================================================================

int main(int argc, char** argv)
{
    vm_init();

    if (argc == 2)
    {
        printf("reading file \"%s\"\n", argv[1]);

        FILE* file = fopen(argv[1], "r");

        if (!file)
        {
            printf("failed to open file\n");
            return -1;
        }

        // Get the file size in bytes
        fseek(file, 0, SEEK_END);
        size_t len = ftell(file);
        fseek(file, 0, SEEK_SET);

        printf("%ld bytes\n", len);

        // Allocate the string object
        string_t* str = string_alloc(len);

        // Read into the allocated buffer
        fread(str->data, 1, len, file);

        // Close the input file
        fclose(file);

        // Create an input stream object
        input_t input = input_from_string(str);

        // Until the end of the input is reached
        while (!input_eof(&input))
        {
            // Parse one expression
            heapptr_t expr = parseExpr(&input);




            // TODO:
            //evalExpr()




        }
    }

    return 0;
}

