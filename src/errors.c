#include <stdlib.h>
#include <string.h>
#include "errors.h"

/**
 * This can be used by any libmtp-intrinsic code that
 * need to stack up an error on the stack. You are only
 * supposed to add errors to the error stack using this
 * function, do not create and reference error entries
 * directly.
 */
static void add_error_to_errorstack(LIBMTP_mtpdevice_t *device,
                                    LIBMTP_error_number_t errornumber,
                                    char const * const error_text)
{
    LIBMTP_error_t *newerror;

    if (device == NULL) {
        LIBMTP_ERROR("LIBMTP PANIC: Trying to add error to a NULL device!\n");
        return;
    }
    newerror = (LIBMTP_error_t *) malloc(sizeof(LIBMTP_error_t));
    newerror->errornumber = errornumber;
    newerror->error_text = strdup(error_text);
    newerror->next = NULL;
    if (device->errorstack == NULL) {
        device->errorstack = newerror;
    } else {
        LIBMTP_error_t *tmp = device->errorstack;

        while (tmp->next != NULL)
            tmp = tmp->next;
        tmp->next = newerror;
    }
}

/**
 * Adds an error from the PTP layer to the error stack.
 */
static void add_ptp_error_to_errorstack(LIBMTP_mtpdevice_t *device,
                                        uint16_t ptp_error,
                                        char const * const error_text)
{
    if (device == NULL) {
        LIBMTP_ERROR("LIBMTP PANIC: Trying to add PTP error to a NULL device!\n");
        return;
    }
    char outstr[256];
    snprintf(outstr, sizeof(outstr), "PTP Layer error %04x: %s", ptp_error, error_text);
    add_error_to_errorstack(device, LIBMTP_ERROR_PTP_LAYER, outstr);

    snprintf(outstr, sizeof(outstr), "Error %04x: %s", ptp_error, ptp_strerror(ptp_error));
    add_error_to_errorstack(device, LIBMTP_ERROR_PTP_LAYER, outstr);
}

