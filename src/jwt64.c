#include "jwt64.h"
#include "base64.h"
#include "rs256.h"
#include "sha256.h"
#include <openssl/evp.h>
#include <stdlib.h>
#include <string.h>

char *newJwt(const char *header, const char *payload, const char *private_key) {
  char *h = base64url_encode(header, strlen(header));
  char *p = base64url_encode(payload, strlen(payload));

  int hp_size = strlen(h) + 1 + strlen(p) + 1;
  char *hp = (char *)malloc(hp_size);

  int nb = sprintf(hp, "%s.%s", h, p);
  if (nb != hp_size - 1) {
    return NULL;
  }

  unsigned char hash[EVP_MAX_MD_SIZE];
  unsigned int hash_length;

  if (!hashSha256(hp, hash, &hash_length)) {
    return NULL;
  };

  unsigned int sign_length;
  char *signature = signRs256(private_key, hash, hash_length, &sign_length);
  if (signature == NULL) {
    return NULL;
  }

  char *s = base64url_encode(signature, sign_length);
  char *jwt = (char *)malloc(strlen(h) + 1 + strlen(p) + 1 + strlen(s) + 1);
  sprintf(jwt, "%s.%s.%s", h, p, s);

  free(s);
  free(h);
  free(p);
  free(hp);
  free(signature);

  return jwt;
}
