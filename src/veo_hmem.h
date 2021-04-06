/**
 * @file veo_hmem.h
 */
#ifndef _VEO_HMEM_H_
#define _VEO_HMEM_H_

#ifdef __cplusplus
extern "C" {
#endif
int veo_is_ve_addr(const void *);
void *veo_get_hmem_addr(void *);
#ifdef __cplusplus
} // extern "C"
#endif
#endif
