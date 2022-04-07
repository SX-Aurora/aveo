/**
 * @file   veo_veshm_defs.h
 * @brief  Header file for VESHM
 */

#ifndef _VEO_IVED_VESHM_H
#define _VEO_IVED_VESHM_H

enum VESHM_mode_flag {
        VE_SHM_RO               = 0x0000000000000001LL,
        VE_PCISYNC              = 0x0000000000000002LL,
        VE_REGISTER_PCI         = 0x0000000000000004LL,
        VE_MAP_256MB            = 0x0000000000000010LL,
};

enum VESHM_additional_mode_flag {
        VE_REGISTER_NONE        = 0x0000000000001000LL, /* IPC */
        VE_REGISTER_VEMVA       = 0x0000000000002000LL, /* IPC */
        VE_REGISTER_VEHVA       = 0x0000000000004000LL, /* IPC */
        VE_MEM_LOCAL            = 0x0000000000008000LL, /* IPC */
};

#endif
