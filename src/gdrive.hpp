#ifndef _G_DRIVE_HPP_
#define _G_DRIVE_HPP_

#include <psp2/json.h>
#include <psp2/sysmodule.h>

#include "httpnet.h"

struct OAuth2Response {
  char *device_code;
  char *user_code;
  char *verification_url;
  char *expires_in;
  char *interval;
};

typedef struct OAuth2Response OAuth2Response_t;

void freeOAuth2Response(OAuth2Response_t response);

class Allocator : public sce::Json::MemAllocator {
public:
  Allocator() {}
  virtual void *allocateMemory(size_t size, void *unk) override {
    return malloc(size);
  }
  virtual void freeMemory(void *ptr, void *unk) override { free(ptr); }
};

/**
 * In this step, your device sends an HTTP POST request to Google's
 * authorization server, at https://oauth2.googleapis.com/device/code, that
 * identifies your application as well as the access scopes that your
 * application wants to access on the user's behalf. You should retrieve this
 * URL from the Discovery document using the device_authorization_endpoint
 * metadata value. Include the following HTTP request parameters:
 * @return Response from the Google OAuth Server.
 */
HttpResponse_t requestDeviceAndUserCodes();

/**
 * The authorization server will return one of the following responses:
 * Success response: If the request is valid, your response will be a JSON
 * object.
 * Quota exceeded response: If your device code requests have exceeded
 * the quota associated with your client ID, you will receive a 403 response,
 * containing the following error: `"error_code": "rate_limit_exceeded"`
 * In that case, use a backoff strategy to reduce the rate of requests.
 * @param json JSON Response from step 1 (requestDeviceAndUserCodes).
 */
OAuth2Response_t handleTheAuthorizationServerResponse(char *json);

/**
 * Display the `verification_url` and `user_code` obtained in step 2 to the
 * user. Both values can contain any printable character from the US-ASCII
 * character set. The content that you display to the user should instruct the
 * user to navigate to the `verification_url` on a separate device and enter the
 * `user_code`.
 * @param oauth2Response The response from the Google OAuth Server (step 2).
 */
void displayUserCode(OAuth2Response_t oauth2Response);

void pollAuthorizationServer();

#endif
