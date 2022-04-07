#include <stdio.h>
#include <stdint.h>

int64_t buffer = 0xdeadbeefdeadbeef;

int veo_memcpy(uint64_t dst, uint64_t src, uint64_t size)
{
	printf("VE: src = %s\n", src);
	memcpy((void *)dst, (void *)src, size);
	printf("VE: copy data src(%lx) to dst(%lx) size = %d\n", src, dst, size);
	fflush(stdout);
	return 0;
}

int print_mem(uint64_t dst)
{
	printf("VE: dst(%lx) = %s\n", dst, dst);
	fflush(stdout);
	return 0;
}

uint64_t print_buffer()
{
  printf("0x%016lx\n", buffer);
  fflush(stdout);
  return 1;
}
uint64_t print_ui(uint64_t *dst)
{
  printf("VE: %u\n", *dst);
  fflush(stdout);
  (*(uint64_t*)dst)++;
  return 1;
}

uint64_t hello(int i)
{
  printf("Hello, %d\n", i);
  fflush(stdout);
  return i + 1;
}

uint64_t empty_cnt = 0;
uint64_t empty(void)
{
  empty_cnt++;
  return empty_cnt;
}

uint64_t empty_cnt2 = 0;
uint64_t empty2(void)
{
  empty_cnt2++;
  return empty_cnt2;
}
