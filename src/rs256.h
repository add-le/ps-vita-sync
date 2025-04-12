#ifndef _RS256_H_
#define _RS256_H_

/**
 * Sign the given hash (hash with SHA2-256) with RSA Private key.
 * @param private_key Private key (RSA PEM format) use to sign the hash.
 * @param hash_to_sign Hash (hash with SHA2-256) to be sign with RSA.
 * @param hash_to_sign_length Length of the hash to sign.
 * @param sign_length Size of the sign result.
 * @return The signature of the given hash. NULL if failed. Result must be freed
 * when not used anymore.
 */
char *signRs256(const char *private_key, unsigned char *hash_to_sign,
                int hash_to_sign_length, unsigned int *sign_length);

#endif
