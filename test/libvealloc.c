#include <stdio.h>
#include <stdint.h>

int veo_previous_result(uint64_t *result);

uint64_t
init( int *p, int n )
{
  printf("VE init %d at p=%p\n", n, p);
  fflush(stdout);
  for (int i = 0; i < n; i++ )
    p[i] = i;
  return (uint64_t)p;
}

uint64_t
check( int *p, int n )
{
  int i;
  printf("VE check %d at %p\n", n, p);
  fflush( stdout );
  for (i = 0; i < n; i++ ) {
    if ( p[i] != i ) {
      printf("p[%d] = %d\n", i, p[i]);
      break;
    }
  }
  return i < n ? ~0 : 0;
}

uint64_t prevres()
{
  uint64_t prevres = 0;
  int rc;
  rc = veo_prev_req_result(&prevres);
  printf("VE prevres returned %lu %p rc=%d\n", prevres, (void *)prevres, rc);
  fflush(stdout);
  return prevres;
}

