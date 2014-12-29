/*
 *  the DES encrypt | decrypt APIs
 *
 *  write by Forrest.zhang
 */

#ifndef __DES_H__
#define __DES_H__

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif  /* end of __cplusplus */

    extern int des_encrypt(u_int8_t key, const u_int8_t *data, u_int8_t *out);

    extern int des_decrypt(u_int8_t key, const u_int8_t *data, u_int8_t *out);


#ifdef __cplusplus
}
#endif  /* end of __cplusplus */

#endif  /* end of __DES_H__ */
