#include "sha256.h"
#include <openssl/evp.h>
#include <stdlib.h>
#include <string.h>

int hashSha256(const char *data_to_hash, unsigned char *buffer,
               unsigned int *buffer_length) {
  int res;

  const EVP_MD *md = EVP_sha256();
  EVP_MD_CTX *ctx = EVP_MD_CTX_create();

  res = EVP_DigestInit_ex(ctx, md, NULL);
  if (!res) {
    EVP_MD_CTX_destroy(ctx);
    return 0;
  }

  res = EVP_DigestUpdate(ctx, data_to_hash, strlen(data_to_hash));
  if (!res) {
    EVP_MD_CTX_destroy(ctx);
    return 0;
  }

  res = EVP_DigestFinal_ex(ctx, buffer, buffer_length);
  if (!res) {
    EVP_MD_CTX_destroy(ctx);
    return 0;
  }

  EVP_MD_CTX_destroy(ctx);
  return 1;
}
