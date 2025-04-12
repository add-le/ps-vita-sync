#include "base64.h"
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <stdlib.h>
#include <string.h>

char *base64url_encode(const char *buffer, size_t buffer_length) {
  BIO *buff, *b64f;
  BUF_MEM *ptr;

  b64f = BIO_new(BIO_f_base64());
  buff = BIO_new(BIO_s_mem());
  buff = BIO_push(b64f, buff);

  BIO_set_flags(buff, BIO_FLAGS_BASE64_NO_NL);
  BIO_set_close(buff, BIO_CLOSE);
  BIO_write(buff, buffer, buffer_length);
  BIO_flush(buff);

  size_t out_len;
  char *out;

  BIO_get_mem_ptr(buff, &ptr);
  out_len = ptr->length;
  out = (char *)malloc((out_len + 1) * sizeof(char));
  memcpy(out, ptr->data, out_len);
  out[out_len] = '\0';

  BIO_free_all(buff);

  // Convert base64 to base64url
  for (int i = 0; i < out_len; i++) {
    switch (out[i]) {
    case '+':
      out[i] = '-';
      break;
    case '/':
      out[i] = '_';
      break;
    }
  }

  // Remove trailling padding
  for (int i = out_len - 2; i > 0; i--) {
    if (out[i] == '=') {
      out[i] = '\0';
    } else {
      break;
    }
  }

  return out;
}
