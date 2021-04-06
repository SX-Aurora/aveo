#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#include <ve_offload.h>

static struct veo_proc_handle *veo_proc_handle = NULL;

#define MAX_LEN (1024*1024)

int
main()
{
    int myrank;
    int nprocs;
    int nelems = MAX_LEN;

    MPI_Init( 0, 0 );
    MPI_Comm_rank( MPI_COMM_WORLD, &myrank );
    MPI_Comm_size( MPI_COMM_WORLD, &nprocs );

    printf( "myrank = %d, nprocs = %d, started\n", myrank, nprocs );
    fflush( stdout );

    //
    // Create a VE proc
    //
    
    int venode = 0;
    struct veo_proc_handle *proc = veo_proc_create( venode );
    printf( "myrank = %d, nprocs = %d, proc = %16p, created\n", myrank, nprocs, proc );

    struct veo_thr_ctxt    *ctx  = veo_context_open( proc );
    struct veo_args        *argp = veo_args_alloc();

    uint64_t handle = veo_load_library( proc, "./libve.so" );

    if ( handle ) { 
      printf( "myrank = %d, nprocs = %d, handle = %16p, VE code loaded\n", myrank, nprocs, handle );
    }
    else {
      printf( "myrank = %d, nprocs = %d, handle = %16p, VE code not loaded\n", myrank, nprocs, handle );
      exit(1);
    }

    //
    // Allocate VE memory 
    //

    void *vebuf;
    int ret = veo_alloc_hmem( proc, &vebuf, sizeof(int) * nelems );
    if (ret != 0) {
        fprintf(stderr, "veo_alloc_hmem failed: %d", ret);
	exit(1);
    }
    printf( "myrank = %d, nprocs = %d, vebuf = %16p, allocated\n", myrank, nprocs, vebuf );

    //
    // Call VE function 'init' to initialize VE memory
    //
    
    uint64_t rc;
    uint64_t id;

    ret = veo_args_set_hmem(argp, 0, vebuf );
    if (ret != 0) {
        fprintf(stderr, "veo_args_set_hmem failed: %d", ret);
	exit(1);
    }
    veo_args_set_i32( argp, 1, myrank );
    veo_args_set_i32( argp, 2, nelems );

    id = veo_call_async_by_name( ctx, handle, "init", argp );
    veo_call_wait_result( ctx, id, &rc );
    printf( "myrank = %d, nprocs = %d, id = %lu, rc=%p, VE offload\n", myrank, nprocs, id, rc );

    int peer = myrank^1;
    if ( peer >= nprocs ) peer = MPI_PROC_NULL;

    //
    // Call VE function 'check' to check initialized VE memory
    //
    
    veo_args_clear(argp);
    ret = veo_args_set_hmem(argp, 0, vebuf );
    if (ret != 0) {
        fprintf(stderr, "veo_args_set_hmem failed: %d", ret);
	exit(1);
    }
    veo_args_set_i32( argp, 1, myrank );
    veo_args_set_i32( argp, 2, nelems );
    id = veo_call_async_by_name( ctx, handle, "check", argp );
    veo_call_wait_result( ctx, id, &rc );
    printf( "myrank = %d, nprocs = %d, id = %lu, rc=%lu, VE offload\n", myrank, nprocs, id, rc );
    
    MPI_Status        status;
    MPI_Barrier(MPI_COMM_WORLD);   

    //
    // MPI calls on VH with VE memory buffers
    //
    
    if ( myrank%2 == 0) {
        MPI_Send((void *)vebuf, MAX_LEN, MPI_INT, peer, 0, MPI_COMM_WORLD);
    }
    else {
        MPI_Recv((void *)vebuf, MAX_LEN, MPI_INT, peer, 0, MPI_COMM_WORLD, &status );
    }

    MPI_Barrier(MPI_COMM_WORLD);  
    
    //
    // Call VE function 'check' to check VE memory after MPI calls
    //

    veo_args_clear(argp);
    ret = veo_args_set_hmem(argp, 0, vebuf );
    if (ret != 0) {
        fprintf(stderr, "veo_args_set_hmem failed: %d", ret);
	exit(1);
    }
    
    if ( myrank%2 == 0) ret = veo_args_set_i32( argp, 1, myrank );
    if ( myrank%2 == 1) ret = veo_args_set_i32( argp, 1, peer   );
    veo_args_set_i32( argp, 2, nelems );
    id = veo_call_async_by_name( ctx, handle, "check", argp );
    veo_call_wait_result( ctx, id, &rc );
    printf( "myrank = %d, nprocs = %d, id = %lu, rc=%lu, VE offload\n", myrank, nprocs, id, rc );

    // Verify

    uint64_t result;
    MPI_Reduce( &rc, &result, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD );
    if ( myrank == 0 ) {
        printf( "Result : %s\n", result ? "Failed" : "Success" );
        fflush( stdout );
    }
    
    veo_free_hmem(vebuf);
    veo_args_free( argp );
    veo_context_close( ctx );
    veo_proc_destroy( proc );
    
    MPI_Finalize();
    return 0;
}
