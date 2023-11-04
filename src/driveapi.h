#ifndef _DRIVE_API_H_
#define _DRIVE_API_H_

/**
 * Get the access token from OAuth2 Google API.
 * @return The access token from OAuth2. Result must be freed when not used
 * anymore.
 */
char *getOAuth2Token();

/**
 * Get file information from the Google Drive.
 * @param fileId Id of the file.
 * @param fields Information to fetch from the file.
 * @param download Set download mode, to get raw data of the file.
 * @param access_token Access token used to connect to the Google Drive API.
 * @return Information about the file fetch. Result must be freed when not used
 * anymore.
 */
char *getFile(char *fileId, char *fields, bool download, char *access_token);

/**
 * Get the modified time from the gFile (Google Drive File).
 * @param json The JSON from getFile fetch (Using with
 * fields=`?fields=modifiedTime`).
 * @return Modified time in ms.
 */
unsigned long long int getModifiedTime(char *json);

#endif
