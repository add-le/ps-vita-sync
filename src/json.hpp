#ifndef _JSON_HPP_
#define _JSON_HPP_

/**
 * Get the value for a key in JSON object.
 * @param json_root Root value of the JSON.
 * @param key Key to get the value.
 * @return The value associated to the key.
 * @warning Result must be freed when not used anymore.
 */
char *jsonGet(sce::Json::Value &json_root, const char *key);

#endif
