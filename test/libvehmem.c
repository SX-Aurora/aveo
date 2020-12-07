#include <stdio.h>
#include <stdint.h>

int64_t buffer = 0xdeadbeefdeadbeef;

uint64_t print_buffer()
{
  printf("0x%016lx\n", buffer);
  fflush(stdout);
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

uint64_t
init( int *p, int v, int n )
{
    if (!veo_is_ve_addr(p))
        return 0;
    printf("p = %p\n", (void *)p);
    p = (int *)veo_get_virt_addr_ve(p);
    printf("p = %p\n", (void *)p);
    printf( "VE init %d %d\n", v, n );
    fflush( stdout );

    int i;
    for ( i = 0; i < n; i++ ) p[i] = v + 1;

    return (uint64_t)p;
}

uint64_t
check( int *p, int v, int n )
{
    printf("p = %p\n", (void *)p);
    p = (int *)veo_get_virt_addr_ve(p);
    printf("p = %p\n", (void *)p);
    printf( "VE check %d %d\n", v, n );
    fflush( stdout );

    int i;
    for ( i = 0; i < n; i++ ) {
        if ( p[i] != v ) {
            printf("p[%d] = %d\n", i, p[i]);
            break;
        }
    }

    return i < n ? ~0 : 0;
}

uint64_t
test_addr(int *p)
{
    return veo_is_ve_addr(p);
}
