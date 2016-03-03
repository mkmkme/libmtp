#include "storage.h"
#include "errors.h"
#include "ptp.h"

#include <stdlib.h>

/**
 * This function updates all the storage id's of a device and their
 * properties, then creates a linked list and puts the list head into
 * the device struct. It also optionally sorts this list. If you want
 * to display storage information in your application you should call
 * this function, then dereference the device struct
 * (<code>device-&gt;storage</code>) to get out information on the storage.
 *
 * You need to call this everytime you want to update the
 * <code>device-&gt;storage</code> list, for example anytime you need
 * to check available storage somewhere.
 *
 * <b>WARNING:</b> since this list is dynamically updated, do not
 * reference its fields in external applications by pointer! E.g
 * do not put a reference to any <code>char *</code> field. instead
 * <code>strncpy()</code> it!
 *
 * @param device a pointer to the device to get the storage for.
 * @param sortby an integer that determines the sorting of the storage list.
 *        Valid sort methods are defined in libmtp.h with beginning with
 *        LIBMTP_STORAGE_SORTBY_. 0 or LIBMTP_STORAGE_SORTBY_NOTSORTED to not
 *        sort.
 * @return 0 on success, 1 success but only with storage id's, storage
 *        properities could not be retrieved and -1 means failure.
 */
int LIBMTP_Get_Storage(LIBMTP_mtpdevice_t *device, int const sortby)
{
    uint32_t i = 0;
    PTPStorageInfo storageInfo;
    PTPParams *params = (PTPParams *) device->params;
    PTPStorageIDs storageIDs;
    LIBMTP_devicestorage_t *storage = NULL;
    LIBMTP_devicestorage_t *storageprev = NULL;

    if (device->storage != NULL)
        free_storage_list(device);

    if (ptp_getstorageids (params, &storageIDs) != PTP_RC_OK)
        return -1;
    if (storageIDs.n < 1)
        return -1;

    if (!ptp_operation_issupported(params,PTP_OC_GetStorageInfo)) {
        for (i = 0; i < storageIDs.n; i++) {
            storage = (LIBMTP_devicestorage_t *) malloc(sizeof(LIBMTP_devicestorage_t));
            storage->prev = storageprev;
            if (storageprev != NULL)
                storageprev->next = storage;
            if (device->storage == NULL)
                device->storage = storage;

            storage->id = storageIDs.Storage[i];
            storage->StorageType = PTP_ST_Undefined;
            storage->FilesystemType = PTP_FST_Undefined;
            storage->AccessCapability = PTP_AC_ReadWrite;
            storage->MaxCapacity = (uint64_t) -1;
            storage->FreeSpaceInBytes = (uint64_t) -1;
            storage->FreeSpaceInObjects = (uint64_t) -1;
            storage->StorageDescription = strdup("Unknown storage");
            storage->VolumeIdentifier = strdup("Unknown volume");
            storage->next = NULL;

            storageprev = storage;
        }

        free(storageIDs.Storage);
        return 1;

    } else {
        for (i = 0; i < storageIDs.n; i++) {
            uint16_t ret;
            ret = ptp_getstorageinfo(params, storageIDs.Storage[i], &storageInfo);
            if (ret != PTP_RC_OK) {
                add_ptp_error_to_errorstack(device, ret,
                                            "LIBMTP_Get_Storage(): Could not get storage info.");
                if (device->storage != NULL)
                    free_storage_list(device);
                return -1;
            }

            storage = (LIBMTP_devicestorage_t *) malloc(sizeof(LIBMTP_devicestorage_t));
            storage->prev = storageprev;
            if (storageprev != NULL)
                storageprev->next = storage;
            if (device->storage == NULL)
                device->storage = storage;

            storage->id = storageIDs.Storage[i];
            storage->StorageType = storageInfo.StorageType;
            storage->FilesystemType = storageInfo.FilesystemType;
            storage->AccessCapability = storageInfo.AccessCapability;
            storage->MaxCapacity = storageInfo.MaxCapability;
            storage->FreeSpaceInBytes = storageInfo.FreeSpaceInBytes;
            storage->FreeSpaceInObjects = storageInfo.FreeSpaceInImages;
            storage->StorageDescription = storageInfo.StorageDescription;
            storage->VolumeIdentifier = storageInfo.VolumeLabel;
            storage->next = NULL;

            storageprev = storage;
        }

        if (storage != NULL)
            storage->next = NULL;

        sort_storage_by(device,sortby);
        free(storageIDs.Storage);
        return 0;
    }
}

/**
 * Formats device storage (if the device supports the operation).
 * WARNING: This WILL delete all data from the device. Make sure you've
 * got confirmation from the user BEFORE you call this function.
 *
 * @param device a pointer to the device containing the storage to format.
 * @param storage the actual storage to format.
 * @return 0 on success, any other value means failure.
 */
int LIBMTP_Format_Storage(LIBMTP_mtpdevice_t *device,
                          LIBMTP_devicestorage_t *storage)
{
    uint16_t ret;
    PTPParams *params = (PTPParams *) device->params;

    if (!ptp_operation_issupported(params,PTP_OC_FormatStore)) {
        add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL,
                                "LIBMTP_Format_Storage(): device does not support formatting storage.");
        return -1;
    }
    ret = ptp_formatstore(params, storage->id);
    if (ret != PTP_RC_OK) {
        add_ptp_error_to_errorstack(device, ret,
                                    "LIBMTP_Format_Storage(): failed to format storage.");
        return -1;
    }
    return 0;
}

