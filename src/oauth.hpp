#ifndef _OAUTH_HPP_
#define _OAUTH_HPP_

#include <stdlib.h>

#include <psp2/json.h>
#include <psp2/sysmodule.h>

struct OAuthCredentials {
  char *client_id;
  char *client_secret;
};

/**
 * @param client_id The client id loaded from config file.
 * @param client_secret The client secret loaded from config file.
 */
typedef struct OAuthCredentials OAuthCredentials_t;

OAuthCredentials_t loadOAuthCredentials();

void freeOAuthCredentials(OAuthCredentials_t credentials);

#endif
