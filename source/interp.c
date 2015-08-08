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
            // TODO:
            //return value_from_ptr(expr);
        }

        // TODO: use longjmp?
        // we won't expose this to user code? or will we?
        // can use special error value, not accessible to user code
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

