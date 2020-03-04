#include <stdio.h>
#include <stdint.h>

uint64_t hello(int i)
{
  printf("Hello, %d\n", i);
  fflush(stdout);
#if 0
  // divide on purpose by zero
  return 1/(i - 42);
#else
  // access some hopefully unmapped location
  return *((uint64_t *)&hello + 100000000);
#endif
}

