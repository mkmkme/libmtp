#include <stdint.h>

#include "storage.h"
#include "errors.h"

/**
 * @defgroup basic The basic device management API.
 * @{
 */

/**
 * Main MTP device object struct
 */
struct LIBMTP_mtpdevice_struct {
    /**
     * Object bitsize, typically 32 or 64.
     */
    uint8_t object_bitsize;
    /**
     * Parameters for this device, must be cast into
     * \c (PTPParams*) before internal use.
     */
    void *params;
    /**
     * USB device for this device, must be cast into
     * \c (PTP_USB*) before internal use.
     */
    void *usbinfo;
    /**
     * The storage for this device, do not use strings in here without
     * copying them first, and beware that this list may be rebuilt at
     * any time.
     * @see LIBMTP_Get_Storage()
     */
    LIBMTP_devicestorage_t *storage;
    /**
     * The error stack. This shall be handled using the error getting
     * and clearing functions, not by dereferencing this list.
     */
    LIBMTP_error_t *errorstack;
    /** The maximum battery level for this device */
    uint8_t maximum_battery_level;
    /** Default music folder */
    uint32_t default_music_folder;
    /** Default playlist folder */
    uint32_t default_playlist_folder;
    /** Default picture folder */
    uint32_t default_picture_folder;
    /** Default video folder */
    uint32_t default_video_folder;
    /** Default organizer folder */
    uint32_t default_organizer_folder;
    /** Default ZENcast folder (only Creative devices...) */
    uint32_t default_zencast_folder;
    /** Default Album folder */
    uint32_t default_album_folder;
    /** Default Text folder */
    uint32_t default_text_folder;
    /** Per device iconv() converters, only used internally */
    void *cd;
    /** Extension list */
    LIBMTP_device_extension_t *extensions;
    /** Whether the device uses caching, only used internally */
    int cached;

    /** Pointer to next device in linked list; NULL if this is the last device */
    LIBMTP_mtpdevice_t *next;
};

typedef struct LIBMTP_mtpdevice_struct LIBMTP_mtpdevice_t; /**< @see LIBMTP_mtpdevice_struct */

LIBMTP_error_number_t LIBMTP_Detect_Raw_Devices(LIBMTP_raw_device_t **devices,
                                                int *numdevs);

int LIBMTP_Check_Specific_Device(int busno, int devno);

LIBMTP_mtpdevice_t *LIBMTP_Open_Raw_Device(LIBMTP_raw_device_t *rawdevice);

LIBMTP_mtpdevice_t *LIBMTP_Open_Raw_Device_Uncached(LIBMTP_raw_device_t *rawdevice);

/* Begin old, legacy interface */
LIBMTP_mtpdevice_t *LIBMTP_Get_First_Device(void);
LIBMTP_error_number_t LIBMTP_Get_Connected_Devices(LIBMTP_mtpdevice_t **device_list);
uint32_t LIBMTP_Number_Devices_In_List(LIBMTP_mtpdevice_t *device_list);
void LIBMTP_Release_Device_List(LIBMTP_mtpdevice_t *device_list);
/* End old, legacy interface */

void LIBMTP_Release_Device(LIBMTP_mtpdevice_t *device);

void LIBMTP_Dump_Device_Info(LIBMTP_mtpdevice_t *device);

int LIBMTP_Reset_Device(LIBMTP_mtpdevice_t *device);

char *LIBMTP_Get_Manufacturername(LIBMTP_mtpdevice_t *device);

char *LIBMTP_Get_Modelname(LIBMTP_mtpdevice_t *device);

char *LIBMTP_Get_Serialnumber(LIBMTP_mtpdevice_t *device);

char *LIBMTP_Get_Deviceversion(LIBMTP_mtpdevice_t *device);

char *LIBMTP_Get_Friendlyname(LIBMTP_mtpdevice_t *device);

int LIBMTP_Set_Friendlyname(LIBMTP_mtpdevice_t *device,
                            char const * const friendlyname);

char *LIBMTP_Get_Syncpartner(LIBMTP_mtpdevice_t *device);

int LIBMTP_Set_Syncpartner(LIBMTP_mtpdevice_t *device,
                           char const * const syncpartner);

int LIBMTP_Get_Batterylevel(LIBMTP_mtpdevice_t *device,
                            uint8_t * const maximum_level,
                            uint8_t * const current_level);

int LIBMTP_Get_Secure_Time(LIBMTP_mtpdevice_t *device,
                           char ** const sectime);

int LIBMTP_Get_Device_Certificate(LIBMTP_mtpdevice_t *device,
                                  char ** const devcert);

int LIBMTP_Get_Supported_Filetypes(LIBMTP_mtpdevice_t *device,
                                   uint16_t ** const filetypes,
                                   uint16_t * const length);

int LIBMTP_Check_Capability(LIBMTP_mtpdevice_t *device,
                            LIBMTP_devicecap_t cap);

LIBMTP_error_t *LIBMTP_Get_Errorstack(LIBMTP_mtpdevice_t *device);

void LIBMTP_Clear_Errorstack(LIBMTP_mtpdevice_t *device);

void LIBMTP_Dump_Errorstack(LIBMTP_mtpdevice_t *device);

/**
 * @}
 */
