#include "rs256.h"
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <stdio.h>

char *signRs256(const char *private_key, unsigned char *hash_to_sign,
                int hash_to_sign_length, unsigned int *sign_length) {

  BIO *bio = BIO_new_mem_buf(private_key, -1);
  if (bio == NULL) {
    return NULL;
  }

  RSA *rsa = PEM_read_bio_RSAPrivateKey(bio, NULL, NULL, NULL);
  if (rsa == NULL) {
    return NULL;
  }

  unsigned char *sigret = (unsigned char *)malloc(RSA_size(rsa));
  if (sigret == NULL) {
    return NULL;
  }

  RSA_sign(NID_sha256, hash_to_sign, hash_to_sign_length, sigret, sign_length,
           rsa);

  return (char *)sigret;
}
