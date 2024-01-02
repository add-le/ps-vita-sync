#ifndef _G_DRIVE_H_
#define _G_DRIVE_H_

#include <psp2/json.h>
#include <psp2/sysmodule.h>

#include "httpnet.h"

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
void handleTheAuthorizationServerResponse(char *json);

#endif
