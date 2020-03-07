#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "ve_offload.h"
#include "veo_time.h"

int main(int argc, char *argv[])
{
	int err = 0;

        struct veo_proc_handle *proc;

        proc = veo_proc_create(-1);
        printf("proc = %p\n", (void *)proc);
        if (proc == NULL)
		return -1;

        if (argc > 1) {
          for (int i = 1; i < argc; i++) {
            uint64_t sym = veo_get_sym(proc, 0, argv[i]);
            printf("'%s' sym = %p\n", argv[i], (void *)sym);
          }
        }

        err = veo_proc_destroy(proc);
        return 0;
}

