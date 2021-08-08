
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define IN_BUILD_DLL 1

#include "lib_dynamic.h"

int lib_dynamic_add(int a, int b)
{
   return a + b;
}
int lib_dynamic_plus(int a)
{
   return lib_dynamic_add(a , 1);
}

void* lib_dynamic_create(int n)
{
   return malloc(n);
}
void lib_dynamic_destroy(void* o)
{
   free(o);
}
