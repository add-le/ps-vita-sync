#ifndef _APP_CONFIG_H_
#define _APP_CONFIG_H_

#define APP_ID "SYNC00055"

struct AppConfig {
  char *iss;
  char *kid;
};

/**
 * @param iss The issuer loaded from config file.
 * @param kid The app key id loaded from config file.
 */
typedef struct AppConfig AppConfig_t;

/**
 * Load the different config object from the config file.
 * @return Config object loaded.
 */
AppConfig_t *loadConfig();

/**
 * Free the allocated config object.
 * @param config The config object to free.
 */
void freeConfig(AppConfig_t *config);

#endif
