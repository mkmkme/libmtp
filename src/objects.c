
/**
 * This function deletes a single file, track, playlist, folder or
 * any other object off the MTP device, identified by the object ID.
 *
 * If you delete a folder, there is no guarantee that the device will
 * really delete all the files that were in that folder, rather it is
 * expected that they will not be deleted, and will turn up in object
 * listings with parent set to a non-existant object ID. The safe way
 * to do this is to recursively delete all files (and folders) contained
 * in the folder, then the folder itself.
 *
 * @param device a pointer to the device to delete the object from.
 * @param object_id the object to delete.
 * @return 0 on success, any other value means failure.
 */
int LIBMTP_Delete_Object(LIBMTP_mtpdevice_t *device,
                         uint32_t id)
{
    uint16_t ret;
    PTPParams *params = (PTPParams *) device->params;

    ret = ptp_deleteobject(params, id, 0);
    if (ret != PTP_RC_OK) {
        add_ptp_error_to_errorstack(device, ret, "LIBMTP_Delete_Object(): could not delete object.");
        return -1;
    }

    return 0;
}


/**
 * THIS FUNCTION IS DEPRECATED. PLEASE UPDATE YOUR CODE IN ORDER
 * NOT TO USE IT.
 *
 * @see LIBMTP_Set_File_Name()
 * @see LIBMTP_Set_Track_Name()
 * @see LIBMTP_Set_Folder_Name()
 * @see LIBMTP_Set_Playlist_Name()
 * @see LIBMTP_Set_Album_Name()
 */
int LIBMTP_Set_Object_Filename(LIBMTP_mtpdevice_t *device,
                               uint32_t object_id,
                               const char* newname)
{
    int ret;
    LIBMTP_file_t *file;

    file = LIBMTP_Get_Filemetadata(device, object_id);

    if (file == NULL) {
        add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL,
                                "LIBMTP_Set_Object_Filename(): could not get file metadata for target object.");
        return -1;
    }

    ret = set_object_filename(device, object_id, map_libmtp_type_to_ptp_type(file->filetype), &newname);

    free(file);

    return ret;
}


int LIBMTP_GetPartialObject(LIBMTP_mtpdevice_t *device,
                            uint32_t const id,
                            uint64_t offset,
                            uint32_t maxbytes,
                            unsigned char **data,
                            unsigned int *size)
{
    PTPParams *params = (PTPParams *) device->params;
    uint16_t ret;

    if (!ptp_operation_issupported(params, PTP_OC_ANDROID_GetPartialObject64)) {
        if  (!ptp_operation_issupported(params, PTP_OC_GetPartialObject)) {
            add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL,
                                    "LIBMTP_GetPartialObject: PTP_OC_GetPartialObject not supported");
            return -1;
        }

        if (offset >> 32 != 0) {
            add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL,
                                    "LIBMTP_GetPartialObject: PTP_OC_GetPartialObject only supports 32bit offsets");
            return -1;
        }

        ret = ptp_getpartialobject(params, id, (uint32_t)offset, maxbytes, data, size);
    } else
        ret = ptp_android_getpartialobject64(params, id, offset, maxbytes, data, size);
    if (ret == PTP_RC_OK)
        return 0;
    return -1;
}

int LIBMTP_SendPartialObject(LIBMTP_mtpdevice_t *device,
                             uint32_t const id,
                             uint64_t offset,
                             unsigned char *data,
                             unsigned int size)
{
    PTPParams *params = (PTPParams *) device->params;
    uint16_t ret;

    if (!ptp_operation_issupported(params, PTP_OC_ANDROID_SendPartialObject)) {
        add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL,
                                "LIBMTP_SendPartialObject: PTP_OC_ANDROID_SendPartialObject not supported");
        return -1;
    }

    ret = ptp_android_sendpartialobject(params, id, offset, data, size);
    if (ret == PTP_RC_OK)
        return 0;
    return -1;
}


int LIBMTP_BeginEditObject(LIBMTP_mtpdevice_t *device, uint32_t const id)
{
    PTPParams *params = (PTPParams *) device->params;
    uint16_t ret;

    if (!ptp_operation_issupported(params, PTP_OC_ANDROID_BeginEditObject)) {
        add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL,
                                "LIBMTP_BeginEditObject: PTP_OC_ANDROID_BeginEditObject not supported");
        return -1;
    }

    ret = ptp_android_begineditobject(params, id);
    if (ret == PTP_RC_OK)
        return 0;
    return -1;
}


int LIBMTP_EndEditObject(LIBMTP_mtpdevice_t *device, uint32_t const id)
{
    PTPParams *params = (PTPParams *) device->params;
    uint16_t ret;

    if (!ptp_operation_issupported(params, PTP_OC_ANDROID_EndEditObject)) {
        add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL,
                                "LIBMTP_EndEditObject: PTP_OC_ANDROID_EndEditObject not supported");
        return -1;
    }

    ret = ptp_android_endeditobject(params, id);
    if (ret == PTP_RC_OK) {
        /* update cached object properties if metadata cache exists */
        update_metadata_cache(device, id);
        return 0;
    }
    return -1;
}


int LIBMTP_TruncateObject(LIBMTP_mtpdevice_t *device,
                          uint32_t const id,
                          uint64_t offset)
{
    PTPParams *params = (PTPParams *) device->params;
    uint16_t ret;

    if (!ptp_operation_issupported(params, PTP_OC_ANDROID_TruncateObject)) {
        add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL,
                                "LIBMTP_TruncateObject: PTP_OC_ANDROID_TruncateObject not supported");
        return -1;
    }

    ret = ptp_android_truncate(params, id, offset);
    if (ret == PTP_RC_OK)
        return 0;
    return -1;
}

