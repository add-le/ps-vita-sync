#ifndef _HTTP_NET_H_
#define _HTTP_NET_H_

#include <stdlib.h>

struct HttpHeader {
  char *key;
  char *value;
};

/**
 * @param key The header key.
 * @param value The value of the header key.
 */
typedef struct HttpHeader HttpHeader_t;

/** Init the HTTP NET Module.*/
void httpNetInit();

/**
 * Send HTTP GET Request.
 * @param url The url to fetch.
 * @param headers Array of headers.
 * @param headers_length Size of the array of headers.
 * @return Result of the GET Request. Result must be freed when not used
 * anymore.
 */
char *httpGet(char *url, HttpHeader_t *headers, size_t headers_length);

/**
 * Send HTTP POST Request.
 * @param url The url to fetch.
 * @return Result of the POST Request. Result must be freed when not used
 * anymore.
 */
char *httpPost(char *url);

#endif
