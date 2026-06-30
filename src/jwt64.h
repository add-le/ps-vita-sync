#ifndef _JWT64_H_
#define _JWT64_H_

/**
 * Generate a jwt from its header and payload, sign it using the private key
 * (RS256).
 * @param header JSON formatted header.
 * @param payload JSON formatted payload.
 * @param private_key RSA private key in PKCS#8 format.
 * @return The jwt base64Url encoded. NULL if failed.
 */
char *newJwt(const char *header, const char *payload, const char *private_key);

#endif
