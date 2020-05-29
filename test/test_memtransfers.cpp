#include <iostream>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <ve_offload.h>

#define CHECK(err) if(err != VEO_COMMAND_OK) { std::cerr << "ERROR" << std::endl; exit(1); }

int main(int argc, char** argv) {
	auto proc = veo_proc_create(0);	assert(proc);
	auto ctx = veo_context_open(proc); assert(ctx);

	uint64_t d_ptr = 0;
	CHECK(veo_alloc_mem(proc, &d_ptr, 1024)); assert(d_ptr);

	const char* h_ptr = "0123456789COFFEEDEADBEEF";
	char r_ptr[1024];

	for(size_t len = 1; len < strlen(h_ptr); len++) {
		memset(r_ptr, '?', sizeof(r_ptr));
		CHECK(veo_write_mem(proc, d_ptr, h_ptr, len));
		CHECK(veo_read_mem(proc, r_ptr, d_ptr, len));

		std::cout << "Len " << len << " : ";
                for(size_t i = 0; i < strlen(h_ptr); i++) {
                  if(r_ptr[i] == 0)
                    std::cout << "\\0";
                  else
                    std::cout << r_ptr[i];
                }
		std::cout << std::endl;
	}

	CHECK(veo_proc_destroy(proc));


	return 0;
}
