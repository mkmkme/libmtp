#ifndef LIBMTP_STORAGE_H
#define LIBMTP_STORAGE_H

#include <stdint.h>

#define LIBMTP_STORAGE_SORTBY_NOTSORTED 0
#define LIBMTP_STORAGE_SORTBY_FREESPACE 1
#define LIBMTP_STORAGE_SORTBY_MAXSPACE  2

struct LIBMTP_mtpdevice_struct;

typedef struct LIBMTP_devicestorage_struct LIBMTP_devicestorage_t; /**< @see LIBMTP_devicestorage_t */

/**
 * LIBMTP Device Storage structure
 */
struct LIBMTP_devicestorage_struct {
    uint32_t id; /**< Unique ID for this storage */
    uint16_t StorageType; /**< Storage type */
    uint16_t FilesystemType; /**< Filesystem type */
    uint16_t AccessCapability; /**< Access capability */
    uint64_t MaxCapacity; /**< Maximum capability */
    uint64_t FreeSpaceInBytes; /**< Free space in bytes */
    uint64_t FreeSpaceInObjects; /**< Free space in objects */
    char *StorageDescription; /**< A brief description of this storage */
    char *VolumeIdentifier; /**< A volume identifier */
    LIBMTP_devicestorage_t *next; /**< Next storage, follow this link until NULL */
    LIBMTP_devicestorage_t *prev; /**< Previous storage */
};

int LIBMTP_Get_Storage(struct LIBMTP_mtpdevice_struct *device, int const sortby);
int LIBMTP_Format_Storage(struct LIBMTP_mtpdevice_struct *device, LIBMTP_devicestorage_t *storage);

#endif

