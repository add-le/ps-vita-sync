#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <stdarg.h>
#include <stdio.h>

#include "debugScreen.h"

#define printf psvDebugScreenPrintf

/**
 * TODO: DOC
 */
int logger_printf(const char *format, ...);

/**
 * TODO: Implementations
 */
int logger_trace(const char *format, ...); // White
int logger_info(const char *format, ...);  // Green
int logger_warn(const char *format, ...);  // Yellowish
int logger_err(const char *format, ...);   // Red

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
