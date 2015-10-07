/**
Zeta core parser implementation

This parser is used to parse the runtime library, the self-hosted Zeta parser
and the Zeta JIT compiler.

The AST node structs are mapped onto objects accessible from the Zeta
language.
*/

#ifndef __PARSER_H__
#define __PARSER_H__

#include <stdbool.h>
#include "vm.h"

/// Shape indices for AST nodes
/// These are initialized in init_parser(), see parser.c
extern shapeidx_t SHAPE_AST_CONST;
extern shapeidx_t SHAPE_AST_REF;
extern shapeidx_t SHAPE_AST_DECL;
extern shapeidx_t SHAPE_AST_BINOP;
extern shapeidx_t SHAPE_AST_UNOP;
extern shapeidx_t SHAPE_AST_SEQ;
extern shapeidx_t SHAPE_AST_IF;
extern shapeidx_t SHAPE_AST_CALL;
extern shapeidx_t SHAPE_AST_FUN;

/**
Source position information
*/
typedef struct
{
    uint32_t lineNo;

    uint32_t colNo;

} srcpos_t;

/**
Input stream, character/token stream for parsing functions
*/
typedef struct
{
    /// Internal string (hosted heap)
    string_t* str;

    /// Current index
    uint32_t idx;

    /// Current source position
    srcpos_t pos;

    /// Error text, if a parse error was encountered
    const char* error_str;

} input_t;

/**
Constant value AST node
Used for integers, floats and booleans
*/
typedef struct
{
    shapeidx_t shape;

    value_t val;

} ast_const_t;

/**
Variable reference node
*/
typedef struct
{
    shapeidx_t shape;

    /// Identifier name string
    string_t* name_str;

    /// Local index
    uint32_t idx;

} ast_ref_t;

/**
Variable/constant declaration node
*/
typedef struct
{
    shapeidx_t shape;

    /// Identifier name string
    string_t* name_str;

    /// Local index
    uint32_t idx;

    /// Constant flag
    bool cst;

} ast_decl_t;

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

/// Operator definitions
const opinfo_t OP_MEMBER;
const opinfo_t OP_INDEX;
const opinfo_t OP_NEG;
const opinfo_t OP_NOT;
const opinfo_t OP_ADD;
const opinfo_t OP_SUB;
const opinfo_t OP_MUL;
const opinfo_t OP_DIV;
const opinfo_t OP_MOD;
const opinfo_t OP_LT;
const opinfo_t OP_LE;
const opinfo_t OP_GT;
const opinfo_t OP_GE;
const opinfo_t OP_IN;
const opinfo_t OP_INST_OF;
const opinfo_t OP_EQ;
const opinfo_t OP_NE;
const opinfo_t OP_BIT_AND;
const opinfo_t OP_BIT_XOR;
const opinfo_t OP_BIT_OR;
const opinfo_t OP_AND;
const opinfo_t OP_OR;
const opinfo_t OP_ASSG;

/**
Unary operator AST node
*/
typedef struct
{
    shapeidx_t shape;

    const opinfo_t* op;

    heapptr_t expr;

} ast_unop_t;

/**
Binary operator AST node
*/
typedef struct
{
    shapeidx_t shape;

    const opinfo_t* op;

    heapptr_t left_expr;
    heapptr_t right_expr;

} ast_binop_t;

/**
Sequence or block of expressions
*/
typedef struct
{
    shapeidx_t shape;

    // List of expressions
    array_t* expr_list;

} ast_seq_t;

/**
If expression AST node
*/
typedef struct
{
    shapeidx_t shape;

    heapptr_t test_expr;

    heapptr_t then_expr;
    heapptr_t else_expr;

} ast_if_t;

/**
Function call AST node
*/
typedef struct
{
    shapeidx_t shape;

    /// Function to be called
    heapptr_t fun_expr;

    /// Argument expressions
    array_t* arg_exprs;

} ast_call_t;

/**
Function expression node
*/
typedef struct
{
    shapeidx_t shape;

    // List of parameter names (strings)
    array_t* param_names;

    /// Function body expression
    heapptr_t body_expr;

} ast_fun_t;

char* srcpos_to_str(srcpos_t pos, char* buf);

input_t input_from_string(string_t* str);
bool input_eof(input_t* input);
char input_peek_ch(input_t* input);
void input_eat_ws(input_t* input);

void parser_init();
heapptr_t parse_expr(input_t* input);
heapptr_t parse_unit(input_t* input);

void test_parser();

#endif

