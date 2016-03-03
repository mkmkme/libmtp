#ifndef LIBMTP_ERRORS_H
#define LIBMTP_ERRORS_H

#include <stdint.h>

struct LIBMTP_mtpdevice_struct;

typedef struct LIBMTP_error_struct LIBMTP_error_t; /**< @see LIBMTP_error_struct */

/**
 * These are the numbered error codes. You can also
 * get string representations for errors.
 */
typedef enum {
    LIBMTP_ERROR_NONE,
    LIBMTP_ERROR_GENERAL,
    LIBMTP_ERROR_PTP_LAYER,
    LIBMTP_ERROR_USB_LAYER,
    LIBMTP_ERROR_MEMORY_ALLOCATION,
    LIBMTP_ERROR_NO_DEVICE_ATTACHED,
    LIBMTP_ERROR_STORAGE_FULL,
    LIBMTP_ERROR_CONNECTING,
    LIBMTP_ERROR_CANCELLED
} LIBMTP_error_number_t;

/**
 * A data structure to hold errors from the library.
 */
struct LIBMTP_error_struct {
    LIBMTP_error_number_t errornumber;
    char *error_text;
    LIBMTP_error_t *next;
};

void add_error_to_errorstack(struct LIBMTP_mtpdevice_struct *device,
                             LIBMTP_error_number_t errornumber,
                             char const * const error_text
);

void add_ptp_error_to_errorstack(struct LIBMTP_mtpdevice_struct *device,
                                 uint16_t ptp_error,
                                 char const * const error_text);

#endif

