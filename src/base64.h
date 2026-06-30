#ifndef _BASE64_H_
#define _BASE64_H_

#include <stdlib.h>

/**
 * @param buffer Binary buffer to convert into base64url.
 * @param buffer_length Size of the buffer.
 * @return The base64url string encoded.
 */
char *base64url_encode(const char *buffer, size_t buffer_length);

#endif
