
#ifndef _PKEY_H_
#define _PKEY_H_

/**
 * Load the RSA Private Key (PKCS#8 Format) from the location
 * ux0:/data/${APP_ID}/pkey.pem.
 * @return The RSA Private Key.
 */
char *loadPkey();

#endif
