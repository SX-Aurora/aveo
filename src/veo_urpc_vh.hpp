#ifndef VEO_URPC_VH_INCLUDE
#define VEO_URPC_VH_INCLUDE

#ifndef __ve__

#include <CallArgs.hpp>
using veo::CallArgs;

namespace veo {

int64_t send_call_nolock(urpc_peer_t *up, uint64_t ve_sp, uint64_t addr,
                         std::vector<uint64_t> const &regs,
                         uint64_t stack_top, size_t stack_size,
                         bool copyin, bool copyout, void *stack_buf);
int unpack_call_result(urpc_mb_t *m, std::function<void(void *)> arg,
                       void *payload, size_t plen, uint64_t *result, void *stack);
int wait_req_result(urpc_peer_t *up, int64_t req, int64_t *result);
int wait_req_ack(urpc_peer_t *up, int64_t req);

} // namespace veo

#endif

#endif //VEO_URPC_VH_INCLUDE
