#include <stdio.h>
#include <string.h>
#include "interp.h"
#include "parser.h"
#include "vm.h"

// TODO: need some kind of environment object
// map variable name strings to values
// for now, no type tags

value_t eval_expr(heapptr_t expr)
{
    // Switch on the expression's tag
    switch (get_tag(expr))
    {
        case TAG_AST_CONST:
        {
            ast_const_t* cst = (ast_const_t*)expr;
            return cst->val;
        }

        case TAG_STRING:
        {
            return value_from_heapptr(expr);
        }

        // Binary operator (e.g. a + b)
        case TAG_AST_BINOP:
        {
            ast_binop_t* binop = (ast_binop_t*)expr;

            value_t v0 = eval_expr(binop->left_expr);
            value_t v1 = eval_expr(binop->right_expr);
            int64_t i0 = v0.word.int64;
            int64_t i1 = v1.word.int64;

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
        }

        // If expression
        case TAG_AST_IF:
        {
            ast_if_t* ifexpr = (ast_if_t*)expr;

            value_t t = eval_expr(ifexpr->test_expr);

            if (t.tag != TAG_FALSE)
                return eval_expr(ifexpr->then_expr);
            else
                return eval_expr(ifexpr->else_expr);
        }

        // TODO: use special error value, not accessible to user code
        // run_error_t
        default:
        printf("unknown tag=%ld\n", get_tag(expr));
        return VAL_FALSE;
    }
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
                "failed to parse expression, at %s\n",
                srcpos_to_str(input.pos, buf)
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

    if (memcmp(&value, &expected, sizeof(value)) != 0)
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
    test_eval_int("7", 7);
    test_eval_true("true");
    test_eval_false("false");

    // Comparisons
    test_eval_true("0 < 5");
    test_eval_true("0 <= 5");
    test_eval_true("0 <= 0");
    test_eval_true("0 == 0");
    test_eval_true("0 != 1");
    test_eval_true("true == true");
    test_eval_false("true == false");

    // If expression
    test_eval_int("if true then 1 else 0", 1);
    test_eval_int("if false then 1 else 0", 0);
    test_eval_int("if 0 < 10 then 7 else 3", 7);







}

