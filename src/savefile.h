#ifndef _SAVEFILE_H_
#define _SAVEFILE_H_

/**
 * Get the last modification time of a file.
 * @param path Path to the file.
 * @return Last modification time in ms.
 */
long long getLastModificationTime(char *path);

/**
 * Download the file from the Google Drive and replace the current savefile.
 * @param fileId File to download.
 * @param path Location where to save the downloaded file.
 * @param access_token The access token to fetch Google Drive API.
 * @return Return 1 on success, error otherwise.
 */
int downloadSavefile(char *fileId, char *path, char *access_token);

/**
 * Upload the current local file to the Google Drive.
 * @param path Path to the savefile.
 * @param fileId File to replace with the upload.
 * @param access_token The access token to fetch Google Drive API.
 * @return Return 1 on success, error otherwise.
 */
int uploadSavefile(char *path, char *fileId, char *access_token);

#endif
