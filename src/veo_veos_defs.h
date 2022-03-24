/**
 * @file veo_veos_defs.h
 */
#ifndef _VEO_VEOS_DEFS_H_
#define _VEO_VEOS_DEFS_H_

/* VE specific system calls */
#define SYS_sysve       316

/* RPC sub commands for VESHM */
enum VESHM_command {
        VESHM_OPEN      = 0x30,
        VESHM_ATTACH,
        VESHM_DETACH,
        VESHM_CLOSE,

        VESHM_PGSIZE    = 0x36,
        VESHM_CHECK_PARTIAL,
};

#endif
