#ifndef _SHA256_H_
#define _SHA256_H_

/**
 * Hash the input data with sha256.
 * @param data_to_hash Data to be hashed.
 * @param buffer Result of the hash.
 * @param buffer_length Length of the result buffer.
 * @return 0 on failed.
 */
int hashSha256(const char *data_to_hash, unsigned char *buffer,
               unsigned int *buffer_length);

#endif
