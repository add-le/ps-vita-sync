#ifndef _TIME_HANDLER_H_
#define _TIME_HANDLER_H_

#include <psp2/io/dirent.h>

/**
 * Convert the PSP2 DateTime format to ms.
 * @param date_time DateTime from PSP2.
 * @return The time converted into ms.
 */
long long psp2DateTimeToMs(SceDateTime date_time);

/**
 * Convert ISO String date (JS Format) into ms.
 * @param iso_string ISO String date to convert.
 * @return The time converted into ms.
 */
unsigned long long int ISOStringToMs(char *iso_string);

#endif
