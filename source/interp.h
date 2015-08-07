#ifndef __INTERP_H__
#define __INTERP_H__

#include "vm.h"

word_t eval_expr(heapptr_t expr);

void test_interp();

#endif

