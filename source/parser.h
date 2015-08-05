#ifndef __PARSER_H__
#define __PARSER_H__

#include "vm.h"

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

/**
Integer AST node
*/
typedef struct
{
    desc_t desc;

    int64_t val;

} ast_int_t;

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

input_t input_from_string(string_t* str);
bool input_eof(input_t* input);
char input_peek_ch(input_t* input);
void input_eat_ws(input_t* input);

heapptr_t parseExpr(input_t* input);

#endif

