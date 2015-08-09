#ifndef __INTERP_H__
#define __INTERP_H__

#include "vm.h"

value_t eval_expr(heapptr_t expr);

value_t eval_str(char* cstr);

void test_interp();

#endif

