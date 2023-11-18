#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <stdarg.h>

/**
 * TODO: DOC
 */
int logger_printf(const char *format, ...);

/**
 * Display panic error human readable log to user.
 * @param errno The error number from psp2 system.
 * @return Number of characters print.
 */
int logger_panic(unsigned int errno);

/** Init the logger module. */
void logger_init();

/**
 * Handle and correctly close the logger.
 * @param status Status code returns to the psp2 system.
 */
void logger_exit(int status);

#endif
