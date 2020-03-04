/*
  gcc -o latency latency.c -I/opt/nec/ve/veos/include -L/opt/nec/ve/veos/lib64    -Wl,-rpath=/opt/nec/ve/veos/lib64,-rpath=`pwd` -lveo -L. -lveo_udma
 
 */

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>


#include <ve_offload.h>
#include <urpc_time.h>

/* variables for VEO demo */
int ve_node_number = 0;
struct veo_proc_handle *proc = NULL;
struct veo_thr_ctxt *ctx = NULL;

int veo_init()
{
	int rc;
	char *env;

	env = getenv("VE_NODE_NUMBER");
	if (env)
		ve_node_number = atoi(env);

	proc = veo_proc_create(-1);
	if (proc == NULL) {
		perror("ERROR: veo_proc_create");
		return -1;
	}

	ctx = veo_context_open(proc);
	if (ctx == NULL) {
		perror("ERROR: veo_context_open");
		return -1;
	}
	return 0;
}

int veo_finish()
{
	int close_status = veo_context_close(ctx);
	printf("close status = %d\n", close_status);
        veo_proc_destroy(proc);
	return 0;
}


int main(int argc, char **argv)
{
	int i, rc, n, peer_id;
	uint64_t ve_buff;
	void *local_buff;
	size_t bsize = 1, res;
	long ts, te;
	double bw;
	int do_send = 1, do_recv = 1;
	
	if (argc == 2)
		bsize = atol(argv[1]);

	rc = veo_init();
	if (rc != 0)
		exit(1);

	local_buff = malloc(bsize);
	// touch local buffer
	//memset(local_buff, 65, bsize);
	for (i = 0; i < bsize/sizeof(long); i++)
		((long *)local_buff)[i] = (long)i;
		
	rc = veo_alloc_mem(proc, &ve_buff, bsize);
	if (rc != 0) {
		printf("veo_alloc_mem failed with rc=%d\n", rc);
		goto finish;
	}

	n = (int)( 3.e4 );
	n > 0 ? n : 1;

	printf("calling veo_write_mem\n");
	ts = get_time_us();
	for (i = 0; i < n; i++)
                 res = veo_write_mem(proc, ve_buff, local_buff, bsize);
	te = get_time_us();
	printf("veo_write_mem n=%d time=%.2fs   latency=%f8.1us\n", n, ((double)(te - ts))/1e6, ((double)(te - ts))/n);

	//overwrite local buffer
	memset(local_buff, 65, bsize);

	printf("calling veo_udma_recv\n");
	ts = get_time_us();
	for (i = 0; i < n; i++)
		res = veo_read_mem(proc, local_buff, ve_buff, bsize);
	te = get_time_us();
	printf("veo_read_mem n=%d time=%.2fs   latency=%f8.1us\n", n, ((double)(te - ts))/1e6, ((double)(te - ts))/n);
	
finish:
	veo_finish();
	exit(0);
}

