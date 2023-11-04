#ifndef _SAVEFILE_H_
#define _SAVEFILE_H_

/**
 * Get the last modification time of a file.
 * @param path Path to the file.
 * @return Last modification time in ms.
 */
unsigned long long int getLastModificationTime(char *path);

/**
 * Download the file from the Google Drive and replace the current savefile.
 * @param fileId File to download.
 * @param access_token The access token to fetch Google Drive API.
 * @return Return 1 on success, error otherwise.
 */
int downloadSavefile(char *fileId, char *access_token);

#endif
