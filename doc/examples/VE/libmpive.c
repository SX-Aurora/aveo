#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <veo_hmem.h>

uint64_t
init( int *p, int v, int n )
{
    printf( "VE init : %d %d %p\n", v, n, p );
    fflush( stdout );
    p = (int *)veo_get_hmem_addr(p);

    int i;
    for ( i = 0; i < n; i++ ) p[i] = v;

    return (uint64_t)p;
}

uint64_t
check( int *p, int v, int n )
{
    printf( "VE init : %d %d %p\n", v, n, p );
    fflush( stdout );
    p = (int *)veo_get_hmem_addr(p);
  
    int i;
    for ( i = 0; i < n; i++ ) {
        if ( p[i] != v ) break;
    }
    
    return i < n ? ~0ULL : 0ULL;
}
