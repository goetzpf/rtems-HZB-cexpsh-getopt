/* $Id$ */

#ifndef DRV_ECDR814_ERR_CODES_H
#define DRV_ECDR814_ERR_CODES_H

#include <stdarg.h>

typedef enum {
	EcErrOK = 0,
	EcError = -1,
	EcErrReadOnly = -2,
	EcErrNoMem = -3,
	EcErrNoDevice = -4,
	EcErrNodeNotFound = -5,
	EcErrNotLeafNode = -6,
	EcErrAD6620NotReset = -7,	/* AD6620 writes only allowed while reset */
	EcErrOutOfRange = -8,		/* Value out of range */
	EcErrTooManyTaps = -9		/* too many taps for total decimation */
	/* if adding error codes, ecStrError must be updated */
} EcErrStat;

/* convert error code to string
 * NOTE: return value points to a
 * static area which must not be modified 
 */
char *
ecStrError(EcErrStat e);

/* how to handle warnings (for now, print to stderr) */
void
ecWarning(EcErrStat, char *message, ...);

#endif
