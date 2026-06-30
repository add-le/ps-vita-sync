# ps-vita-sync

Homebrew to sync savefiles with your drive.

### Step 1 : Google Drive

- You need to setup your google drive to be able to interact with the application.
  - On https://console.cloud.google.com/ create an application 'PS Vita Sync'
  - Go to APIs and Services
  - Go to Credentials
  - Create an API Key (kid)
  - Create a service account
  - Generate RSA Key from KEYS menu
  - Download your private key (pkey)
- Share with the service account, the folder where you save will be sync

### Step 2 : Populate the config files

- On you vita go to ux0/data/SYNC00055
- Create a file named 'pkey.pem', saved your privat key here
  - EOL needs to be UNIX type '\n'
  - the EOF need to be empty last char must be a '-'
- Create a file named 'config.lst" inside the same folder
  - The file format need to be exact following
  - start with the word 'iss' used the tab char ('\t') and write the email of your service account
  - enter a new line UNIX style ('\n')
  - start the second line with 'kid' and used the tab char ('\t') and write the api key here
  - terminate the file with the new line UNIX style ('\n')
  - you should have something like this

```lst
iss\tmail@ps-vita-sync.iam.gserviceaccount.com\n
kid\t${API_KEY_HERE}\n

```

### Step 3 : Enjoy

- Compile & install the vpk file on your vita
- Change the source code according to your game
- Change the fileId of google drive

#### About

`descript.ion` file provided by [@vita-loop4124](https://www.youtube.com/@vita-loop4124)
