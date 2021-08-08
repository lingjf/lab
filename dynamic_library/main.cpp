
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "so/lib_dynamic.h"

int
main(int argc, char* argv[])
{
  ::printf("%d\n", lib_dynamic_plus(1));
  void* p = lib_dynamic_create(100);
  lib_dynamic_destroy(p);
  return 0;
}
