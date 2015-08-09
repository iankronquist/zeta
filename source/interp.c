#include <stdio.h>
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
        }

        // TODO: use special error value, not accessible to user code
        // run_error_t
        default:
        printf("unknown tag=%ld\n", get_tag(expr));
        return VAL_FALSE;
    }
}

void test_eval_str(char* cstr, value_t expected)
{

    // TODO


    // TODO: return value of last expression

}

void test_eval_str_int(char* cstr, int64_t expected)
{
    // TODO
    //test_eval_str();
}

void test_interp()
{
    test_eval_str_int("0", 0);
    test_eval_str_int("7", 7);

    // TODO: boolean expressions



}

