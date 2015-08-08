#include "interp.h"
#include "parser.h"
#include "vm.h"

// TODO: need some kind of environment object
// map variable name strings to values
// for now, no type tags

value_t eval_expr(heapptr_t expr)
{
    tag_t tag = *(tag_t*)expr;

    // Switch on the expression's tag
    switch (tag)
    {
        case TAG_AST_CONST:
        {
            ast_const_t* cst = (ast_const_t*)expr;
            return cst->val;
        }




        // TODO: use longjmp? error value?
        default:
        return VAL_FALSE;
    }
}

void test_eval_expr(char* cstr, value_t expected)
{
    // TODO


}

void test_interp()
{
}

