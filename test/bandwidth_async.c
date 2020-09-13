/*
  gcc -o hello hello.c -I/opt/nec/ve/veos/include -L/opt/nec/ve/veos/lib64 \
   -Wl,-rpath=/opt/nec/ve/veos/lib64 -lveo
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
	proc = veo_proc_create(-1);
	if (proc == NULL) {
		perror("ERROR: veo_proc_create");
		return -1;
	}

#ifdef VEO_DEBUG
	printf("If you want to attach to the VE process, you now have 20s!\n\n"
	       "/opt/nec/ve/bin/gdb -p %d veorun_static\n\n", getpid());
	sleep(20);
#endif
	ctx = veo_context_open(proc);
	if (ctx == NULL) {
		perror("ERROR: veo_context_open");
		return -1;
	}
	return 0;
}

int veo_finish()
{
	veo_context_close(ctx);
	veo_proc_destroy(proc);
	return 0;
}


int main(int argc, char **argv)
{
	int i, rc, n, peer_id;
	uint64_t ve_buff;
	void *local_buff;
	size_t bsize = 1024*1024, res;
	long ts, te;
	double bw;
	int do_send = 1, do_recv = 1;
        int ntransfer = 0;
	
	if (argc == 2) {
		if (strcmp(argv[1], "-h") == 0) {
			printf("Usage:\n");
			printf("%s [<size>]\n");
			printf("%s send|recv <size> [<loops>]]\n");
			return 0;
		} else {
			bsize = atol(argv[1]);
		}
	} else if (argc >= 3) {
		if (strcmp(argv[1], "send") == 0)
			do_recv = 0;
		else if (strcmp(argv[1], "recv") == 0)
			do_send = 0;
                else {
			bsize = atol(argv[1]);
			ntransfer = atoi(argv[2]);
		}
		if (do_send == 0 || do_recv == 0) {
			bsize = atol(argv[2]);
			if (argc > 3)
				ntransfer = atoi(argv[3]);
		}
	}

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
                veo_finish();
                exit(1);
	}

	if (ntransfer > 0)
		n = ntransfer;
	else {
		if (bsize < 512 * 1024)
			n = (int)(100 * 1.e6 / (double)bsize);
		else
			n = (int)(5.0 * 1.e9 / (double)bsize);
		n > 0 ? n : 1;
	}

        uint64_t req[n];
	if (do_send) {
		ts = get_time_us();
		for (i = 0; i < n; i++)
                  req[i] = veo_async_write_mem(ctx, (uint64_t)ve_buff, local_buff, bsize);
		for (i = 0; i < n; i++)
                  veo_call_wait_result(ctx, req[i], &res);
		te = get_time_us();
		bw = (double)bsize * n/((double)(te - ts)/1e6);
		bw = bw / 1e6;
		printf("veo_async_write_mem returned: %lu bw=%f MB/s\n", res, bw);
	}

	//overwrite local buffer
	memset(local_buff, 65, bsize);

	if (do_recv) {
		ts = get_time_us();
		for (i = 0; i < n; i++)
		  req[i] = veo_async_read_mem(ctx, local_buff, (uint64_t)ve_buff, bsize);
		for (i = 0; i < n; i++)
                  veo_call_wait_result(ctx, req[i], &res);
		te = get_time_us();
		bw = (double)bsize * n/((double)(te - ts)/1e6);
		bw = bw / 1e6;
		printf("veo_async_read_mem returned: %lu bw=%f MB/s\n", res, bw);
	}

	if (do_send && do_recv) {
		rc = 0;
		// check local_buff content
		for (i = 0; i < bsize/sizeof(long); i++) {
			if (((long *)local_buff)[i] != (long)i) {
				rc = 1;
				break;
			}
		}
		if (rc)
			printf("Verify error: buffer contains wrong data\n");
		else
			printf("Received data is identical with the sent buffer.\n");
	}

	veo_finish();
	exit(0);
}

