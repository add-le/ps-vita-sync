## Roadmap

- Better logging system
  - Write logs in file
- Better human readable errors
  - Use built in dialog system to show errors
- GUI frame
  - GUI frame to select files and folder to sync
- Simplify GDrive Auth
  - Use GAPI to simplify auth with GDrive

## Notes

- OAuth 2.0 authentification with Google Drive REST API
- Change dectection
  - Use the `files` api in GDrive (`files.create` `files.update` `files.get`)
  - Use the `changes` api in GDrive
- Track file modification in PSP2
  - Use checksum (MD5 or SHA-1)
- Syncronization logic
  - Download new or updated files from GDrive
  - If file is deleted from GDrive, sync is disabled for this file
  - Upload new or updated files from PSP2
  - If file is deleted from PSP2, sync is disabled for this file
- Conflict resolution (manual, let the user choose)
  - Use built-in dialog
  - Timestamp
  - Bytes size of the file
  - (Choose to replace PSP2 version or Cloud version)

## Local change detection using database (sqlite)

- Metadata about file
  - Checksum
  - Size Byte Length
  - Timestamp
  - Bool, shouldSync, shouldntSync...
  - LastModificationDate, CreationDate, InsertedDate, SyncModificationDate
- Need to generate this database (cron...)
