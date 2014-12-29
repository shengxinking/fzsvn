/*
 *
 *
 *
 *
 *
 */

#ifndef KEYFILE_H
#define KEYFILE_H

/* for PEM format Public Key file */
#define KEYFILE_NONE            0
#define KEYFILE_PUBLIC          1
#define KEYFILE_RSA_PRIVATE     2
#define KEYFILE_DSA_PRIVATE     3



extern int keyfile_type(const char *file);

extern int keyfile_rsa_bits(const char *file);

extern int keyfile_dsa_bits(const char *file);


#endif /* end of KEYFILE_H */
