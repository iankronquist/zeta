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
    tag_t tag;

    int64_t val;

} ast_int_t;

/**
Operator information structure
*/
typedef struct
{
    /// Operator string (e.g. "+")
    char* str;

    /// Closing string (optional)
    char* close_str;

    /// Operator arity
    int arity;

    /// Precedence level
    int prec;

    /// Associativity, left-to-right or right-to-left ('l' or 'r')
    char assoc;

    /// Non-associative flag (e.g.: - and / are not associative)
    bool nonassoc;

} opinfo_t;

// Operator definitions
const opinfo_t OP_ADD;

/**
Binary operator AST node
*/
typedef struct
{
    tag_t tag;

    const opinfo_t* op;

    heapptr_t left;
    heapptr_t right;

} ast_binop_t;

/**
If expression AST node
*/
typedef struct
{
    tag_t tag;

    heapptr_t test_expr;

    heapptr_t then_expr;
    heapptr_t else_expr;

} ast_if_t;

/**
Function call AST node
*/
typedef struct
{
    tag_t tag;

    /// Function to be called
    heapptr_t fun_expr;

    /// Argument expressions
    heapptr_t arg_exprs;

} ast_call_t;

/**
Function expression node
*/
typedef struct
{
    tag_t tag;

    // List of parameter names (strings)
    array_t* param_names;

    /// Function body expression
    heapptr_t body_expr;

} ast_fun_t;

input_t input_from_string(string_t* str);
bool input_eof(input_t* input);
char input_peek_ch(input_t* input);
void input_eat_ws(input_t* input);

heapptr_t parse_expr(input_t* input);

void test_parser();

#endif

