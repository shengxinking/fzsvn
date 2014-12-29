/*
 *  APIs for Caesar encrypt and decrypt
 *
 *  write by Forrest.zhang
 */

int caesar_encrypt(
    int         key, 
    u_int8_t    *data, 
    u_int32_t   datalen,
    u_int8_t    *out,
    u_int32_t   outlen,
    u_int8_t    m)
{
    if (key == 0 || !data || !out)
	return -1;

    if (outlen < datalen)
	return -1;

    for (i = 0; i < datalen; i++)
}

int caesar_decrypt(
    int         key,
    u_int8_t    *data, 
    u_int32_t   datalen,
    u_int8_t    *out,
    u_int32_t   outlen)
{
    return caesar_encrypt(-key, data, datalen, out, outlen);
}
