#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "interp.h"
#include "parser.h"
#include "vm.h"

#define MAX_LOCALS 64

/**
Interpreter stack frame
*/
typedef struct
{
    size_t numLocals;

    value_t locals[MAX_LOCALS];

} frame_t;

/**
Evaluate the boolean value of a value
Note: the semantics of boolean evaluation are intentionally
kept strict in the core language.
*/
bool eval_truth(value_t value)
{
    switch (value.tag)
    {
        case TAG_BOOL:
        return value.word.int8 != 0;

        default:
        exit(-1);
    }
}

value_t eval_expr(heapptr_t expr)
{
    // Get the shape of the AST node
    // Note: AST nodes must match the shapes defined in init_parser,
    // otherwise this interpreter can't handle it
    shapeidx_t shape = get_shape(expr);

    if (shape == SHAPE_AST_CONST)
    {
        ast_const_t* cst = (ast_const_t*)expr;
        return cst->val;
    }

    if (shape == SHAPE_STRING)
    {
        return value_from_heapptr(expr, TAG_STRING);
    }

    // Array literal expression
    if (shape == SHAPE_ARRAY)
    {
        array_t* array_expr = (array_t*)expr;

        array_t* val_array = array_alloc(array_expr->len);

        for (size_t i = 0; i < array_expr->len; ++i)
        {
            heapptr_t expr = array_get(array_expr, i).word.heapptr;
            value_t value = eval_expr(expr);
            array_set(val_array, i, value);
        }

        return value_from_heapptr((heapptr_t)val_array, TAG_ARRAY);
    }

    // Binary operator (e.g. a + b)
    if (shape == SHAPE_AST_BINOP)
    {
        ast_binop_t* binop = (ast_binop_t*)expr;

        value_t v0 = eval_expr(binop->left_expr);
        value_t v1 = eval_expr(binop->right_expr);
        int64_t i0 = v0.word.int64;
        int64_t i1 = v1.word.int64;

        if (binop->op == &OP_INDEX)
            return array_get((array_t*)v0.word.heapptr, i1);

        if (binop->op == &OP_ADD)
            return value_from_int64(i0 + i1);
        if (binop->op == &OP_SUB)
            return value_from_int64(i0 - i1);
        if (binop->op == &OP_MUL)
            return value_from_int64(i0 * i1);
        if (binop->op == &OP_DIV)
            return value_from_int64(i0 / i1);
        if (binop->op == &OP_MOD)
            return value_from_int64(i0 % i1);

        if (binop->op == &OP_LT)
            return (i0 < i1)? VAL_TRUE:VAL_FALSE;
        if (binop->op == &OP_LE)
            return (i0 <= i1)? VAL_TRUE:VAL_FALSE;
        if (binop->op == &OP_GT)
            return (i0 > i1)? VAL_TRUE:VAL_FALSE;
        if (binop->op == &OP_GE)
            return (i0 >= i1)? VAL_TRUE:VAL_FALSE;

        if (binop->op == &OP_EQ)
            return (memcmp(&v0, &v1, sizeof(v0)) == 0)? VAL_TRUE:VAL_FALSE;
        if (binop->op == &OP_NE)
            return (memcmp(&v0, &v1, sizeof(v0)) != 0)? VAL_TRUE:VAL_FALSE;

        printf("unimplemented binary operator: %s\n", binop->op->str);
        return VAL_FALSE;
    }

    // Unary operator (e.g.: -x, not a)
    if (shape == SHAPE_AST_UNOP)
    {
        ast_unop_t* unop = (ast_unop_t*)expr;

        value_t v0 = eval_expr(unop->expr);

        if (unop->op == &OP_NEG)
            return value_from_int64(-v0.word.int64);

        if (unop->op == &OP_NOT)
            return eval_truth(v0)? VAL_FALSE:VAL_TRUE;

        printf("unimplemented unary operator: %s\n", unop->op->str);
        return VAL_FALSE;
    }

    // Sequence/block expression
    if (shape == SHAPE_AST_SEQ)
    {
        ast_seq_t* seqexpr = (ast_seq_t*)expr;
        array_t* expr_list = seqexpr->expr_list;

        value_t value;

        for (size_t i = 0; i < expr_list->len; ++i)
        {
            heapptr_t expr = array_get(expr_list, i).word.heapptr;
            value = eval_expr(expr);
        }

        // Return the value of the last expression
        return value;
    }

    // If expression
    if (shape == SHAPE_AST_IF)
    {
        ast_if_t* ifexpr = (ast_if_t*)expr;

        value_t t = eval_expr(ifexpr->test_expr);

        if (eval_truth(t))
            return eval_expr(ifexpr->then_expr);
        else
            return eval_expr(ifexpr->else_expr);
    }

    // Call expression
    if (shape == SHAPE_AST_CALL)
    {
        ast_call_t* callexpr = (ast_call_t*)expr;
        heapptr_t fun_expr = callexpr->fun_expr;
        array_t* arg_exprs = callexpr->arg_exprs;

        if (get_shape(fun_expr) == SHAPE_AST_REF && arg_exprs->len == 1)
        {
            ast_ref_t* fun_ident = (ast_ref_t*)fun_expr;
            char* name_cstr = fun_ident->name_str->data;

            heapptr_t arg_expr = array_get(arg_exprs, 0).word.heapptr;
            value_t arg_val = eval_expr(arg_expr);

            if (strncmp(name_cstr, "println", strlen("println")) == 0)
            {
                value_print(arg_val);
                putchar('\n');
                return VAL_TRUE;
            }
        }

        printf("eval error, unknown function in call expression\n");
        return VAL_FALSE;
    }

    // Function/closure expression
    if (shape == SHAPE_AST_FUN)
    {
        // For now, return the function unchanged
        // Later, we will have closure objects
        return value_from_heapptr(expr, TAG_CLOS);
    }

    printf("eval error, unknown expression type, shapeidx=%d\n", get_shape(expr));
    exit(-1);
}

/// Evaluate the code in a given string
value_t eval_str(char* cstr)
{
    size_t len = strlen(cstr);

    // Allocate a hosted string object
    string_t* str = string_alloc(len);

    strncpy(str->data, cstr, len);

    // Create a parser input stream object
    input_t input = input_from_string(str);

    // Until the end of the input is reached
    for (;;)
    {
        // Parse one expression
        heapptr_t expr = parse_expr(&input);

        if (expr == NULL)
        {
            char buf[64];
            printf(
                "Failed to parse expression %s - %s\n",
                srcpos_to_str(input.pos, buf),
                input.error_str
            );

            return VAL_FALSE;
        }

        // Evaluate the expression
        value_t value = eval_expr(expr);

        // If this is the end of the input, stop
        input_eat_ws(&input);
        if (input_eof(&input))
            return value;
    }
}

void test_eval(char* cstr, value_t expected)
{
    value_t value = eval_str(cstr);

    if (!value_equals(value, expected))
    {
        printf(
            "value doesn't match expected for input:\n%s\n",
            cstr
        );

        exit(-1);
    }
}

void test_eval_int(char* cstr, int64_t expected)
{
    test_eval(cstr, value_from_int64(expected));
}

void test_eval_true(char* cstr)
{
    test_eval(cstr, VAL_TRUE);
}

void test_eval_false(char* cstr)
{
    test_eval(cstr, VAL_FALSE);
}

void test_interp()
{
    test_eval_int("0", 0);
    test_eval_int("1", 1);
    test_eval_int("7", 7);
    test_eval_int("0xFF", 255);
    test_eval_int("0b101", 5);
    test_eval_true("true");
    test_eval_false("false");

    // Arithmetic
    test_eval_int("3 + 2 * 5", 13);
    test_eval_int("-7", -7);
    test_eval_int("-(7 + 3)", -10);
    test_eval_int("3 + -2 * 5", -7);

    // Comparisons
    test_eval_true("0 < 5");
    test_eval_true("0 <= 5");
    test_eval_true("0 <= 0");
    test_eval_true("0 == 0");
    test_eval_true("0 != 1");
    test_eval_true("not false");
    test_eval_true("not not true");
    test_eval_true("true == true");
    test_eval_false("true == false");

    // Arrays
    test_eval_int("[7][0]", 7);
    test_eval_int("[0,1,2][0]", 0);
    test_eval_int("[7+3][0]", 10);

    // Sequence expression
    test_eval_int("{ 2 3 }", 3);
    test_eval_int("{ 2 3+7 }", 10);

    /*
    // If expression
    test_eval_int("if true then 1 else 0", 1);
    test_eval_int("if false then 1 else 0", 0);
    test_eval_int("if 0 < 10 then 7 else 3", 7);
    test_eval_int("if 0 then 1 else 0", 0);
    test_eval_int("if 1 then 777", 777);
    test_eval_int("if 'abc' then 777", 777);
    test_eval_int("if '' then 777 else 0", 0);
    test_eval_int("if not true then 1 else 0", 0);
    */





}

