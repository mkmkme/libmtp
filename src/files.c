/**
 * This creates a new file metadata structure and allocates memory
 * for it. Notice that if you add strings to this structure they
 * will be freed by the corresponding <code>LIBMTP_destroy_file_t</code>
 * operation later, so be careful of using strdup() when assigning
 * strings, e.g.:
 *
 * <pre>
 * LIBMTP_file_t *file = LIBMTP_new_file_t();
 * file->filename = strdup(namestr);
 * ....
 * LIBMTP_destroy_file_t(file);
 * </pre>
 *
 * @return a pointer to the newly allocated metadata structure.
 * @see LIBMTP_destroy_file_t()
 */
LIBMTP_file_t *LIBMTP_new_file_t(void)
{
    LIBMTP_file_t *file = (LIBMTP_file_t *) malloc(sizeof(LIBMTP_file_t));
    if (file == NULL)
        return NULL;
    file->filename = NULL;
    file->item_id = 0;
    file->parent_id = 0;
    file->storage_id = 0;
    file->filesize = 0;
    file->modificationdate = 0;
    file->filetype = LIBMTP_FILETYPE_UNKNOWN;
    file->next = NULL;
    return file;
}

/**
 * This destroys a file metadata structure and deallocates the memory
 * used by it, including any strings. Never use a file metadata
 * structure again after calling this function on it.
 * @param file the file metadata to destroy.
 * @see LIBMTP_new_file_t()
 */
void LIBMTP_destroy_file_t(LIBMTP_file_t *file)
{
    if (file == NULL)
        return;
    free(file->filename);
    free(file);
}

/**
 * This helper function returns a textual description for a libmtp
 * file type to be used in dialog boxes etc.
 * @param intype the libmtp internal filetype to get a description for.
 * @return a string representing the filetype, this must <b>NOT</b>
 *         be free():ed by the caller!
 */
char const * LIBMTP_Get_Filetype_Description(LIBMTP_filetype_t intype)
{
    filemap_t *current;

    current = g_filemap;

    while (current != NULL) {
        if(current->id == intype)
            return current->description;
        current = current->next;
    }

    return "Unknown filetype";
}

/**
* THIS FUNCTION IS DEPRECATED. PLEASE UPDATE YOUR CODE IN ORDER
 * NOT TO USE IT.
 * @see LIBMTP_Get_Filelisting_With_Callback()
 */
LIBMTP_file_t *LIBMTP_Get_Filelisting(LIBMTP_mtpdevice_t *device)
{
    LIBMTP_INFO("WARNING: LIBMTP_Get_Filelisting() is deprecated.\n");
    LIBMTP_INFO("WARNING: please update your code to use LIBMTP_Get_Filelisting_With_Callback()\n");
    return LIBMTP_Get_Filelisting_With_Callback(device, NULL, NULL);
}

/**
 * This returns a long list of all files available
 * on the current MTP device. Folders will not be returned, but abstract
 * entities like playlists and albums will show up as "files". Typical usage:
 *
 * <pre>
 * LIBMTP_file_t *filelist;
 *
 * filelist = LIBMTP_Get_Filelisting_With_Callback(device, callback, data);
 * while (filelist != NULL) {
 *   LIBMTP_file_t *tmp;
 *
 *   // Do something on each element in the list here...
 *   tmp = filelist;
 *   filelist = filelist->next;
 *   LIBMTP_destroy_file_t(tmp);
 * }
 * </pre>
 *
 * If you want to group your file listing by storage (per storage unit) or
 * arrange files into folders, you must dereference the <code>storage_id</code>
 * and/or <code>parent_id</code> field of the returned <code>LIBMTP_file_t</code>
 * struct. To arrange by folders or files you typically have to create the proper
 * trees by calls to <code>LIBMTP_Get_Storage()</code> and/or
 * <code>LIBMTP_Get_Folder_List()</code> first.
 *
 * @param device a pointer to the device to get the file listing for.
 * @param callback a function to be called during the tracklisting retrieveal
 *        for displaying progress bars etc, or NULL if you don't want
 *        any callbacks.
 * @param data a user-defined pointer that is passed along to
 *        the <code>progress</code> function in order to
 *        pass along some user defined data to the progress
 *        updates. If not used, set this to NULL.
 * @return a list of files that can be followed using the <code>next</code>
 *        field of the <code>LIBMTP_file_t</code> data structure.
 *        Each of the metadata tags must be freed after use, and may
 *        contain only partial metadata information, i.e. one or several
 *        fields may be NULL or 0.
 * @see LIBMTP_Get_Filemetadata()
 */
LIBMTP_file_t *LIBMTP_Get_Filelisting_With_Callback(LIBMTP_mtpdevice_t *device,
        LIBMTP_progressfunc_t const callback,
        void const * const data)
{
    uint32_t i = 0;
    LIBMTP_file_t *retfiles = NULL;
    LIBMTP_file_t *curfile = NULL;
    PTPParams *params = (PTPParams *) device->params;

    /* Get all the handles if we haven't already done that */
    if (params->nrofobjects == 0)
        flush_handles(device);

    for (i = 0; i < params->nrofobjects; i++) {
        LIBMTP_file_t *file;
        PTPObject *ob;

        if (callback != NULL)
            callback(i, params->nrofobjects, data);

        ob = &params->objects[i];

        if (ob->oi.ObjectFormat == PTP_OFC_Association)
            /* MTP use this object format for folders which means
             * these "files" will turn up on a folder listing instead. */
            continue;

        /* Look up metadata */
        file = obj2file(device, ob);
        if (file == NULL)
            continue;

        /* Add track to a list that will be returned afterwards. */
        if (retfiles == NULL) {
            retfiles = file;
            curfile = file;
        } else {
            curfile->next = file;
            curfile = file;
        }

        /* Call listing callback */
        /* double progressPercent = (double)i*(double)100.0 / (double)params->handles.n; */

    } /* Handle counting loop */
    return retfiles;
}

/**
 * This function retrieves the contents of a certain folder
 * with id parent on a certain storage on a certain device.
 * The result contains both files and folders.
 * The device used with this operations must have been opened with
 * LIBMTP_Open_Raw_Device_Uncached() or it will fail.
 *
 * NOTE: the request will always perform I/O with the device.
 * @param device a pointer to the MTP device to report info from.
 * @param storage a storage on the device to report info from. If
 *        0 is passed in, the files for the given parent will be
 *        searched across all available storages.
 * @param parent the parent folder id.
 */
LIBMTP_file_t * LIBMTP_Get_Files_And_Folders(LIBMTP_mtpdevice_t *device,
        uint32_t const storage,
        uint32_t const parent)
{
    PTPParams *params = (PTPParams *) device->params;
    PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;
    LIBMTP_file_t *retfiles = NULL;
    LIBMTP_file_t *curfile = NULL;
    PTPObjectHandles currentHandles;
    uint32_t storageid;
    uint16_t ret;
    int i = 0;

    if (device->cached) {
        /* This function is only supposed to be used by devices
         * opened as uncached! */
        LIBMTP_ERROR("tried to use %s on a cached device!\n",  __func__);
        return NULL;
    }

    if (FLAG_BROKEN_GET_OBJECT_PROPVAL(ptp_usb)) {
        /* These devices cannot handle the commands needed for
         * Uncached access! */
        LIBMTP_ERROR("tried to use %s on an unsupported device, this command does not work on all devices "
                     "due to missing low-level support to read information on individual tracks\n", __func__);
        return NULL;
    }

    if (storage == 0)
        storageid = PTP_GOH_ALL_STORAGE;
    else
        storageid = storage;

    ret = ptp_getobjecthandles(params, storageid, PTP_GOH_ALL_FORMATS, parent, &currentHandles);

    if (ret != PTP_RC_OK) {
        add_ptp_error_to_errorstack(device, ret,
                                    "LIBMTP_Get_Files_And_Folders(): could not get object handles.");
        return NULL;
    }

    if (currentHandles.Handler == NULL || currentHandles.n == 0)
        return NULL;

    for (i = 0; i < currentHandles.n; i++) {
        LIBMTP_file_t *file;

        /* Get metadata for one file, if it fails, try next file */
        file = LIBMTP_Get_Filemetadata(device, currentHandles.Handler[i]);
        if (file == NULL)
            continue;

        /* Add track to a list that will be returned afterwards. */
        if (curfile == NULL) {
            curfile = file;
            retfiles = file;
        } else {
            curfile->next = file;
            curfile = file;
        }
    }

    free(currentHandles.Handler);

    /* Return a pointer to the original first file
     * in the big list. */
    return retfiles;
}

/**
 * This function retrieves the metadata for a single file off
 * the device.
 *
 * Do not call this function repeatedly! The file handles are linearly
 * searched O(n) and the call may involve (slow) USB traffic, so use
 * <code>LIBMTP_Get_Filelisting()</code> and cache the file, preferably
 * as an efficient data structure such as a hash list.
 *
 * Incidentally this function will return metadata for
 * a folder (association) as well, but this is not a proper use
 * of it, it is intended for file manipulation, not folder manipulation.
 *
 * @param device a pointer to the device to get the file metadata from.
 * @param fileid the object ID of the file that you want the metadata for.
 * @return a metadata entry on success or NULL on failure.
 * @see LIBMTP_Get_Filelisting()
 */
LIBMTP_file_t *LIBMTP_Get_Filemetadata(LIBMTP_mtpdevice_t *device, uint32_t const fileid)
{
    PTPParams *params = (PTPParams *) device->params;
    uint16_t ret;
    PTPObject *ob;

    /* Get all the handles if we haven't already done that
     * (Only on cached devices.) */
    if (device->cached && params->nrofobjects == 0)
        flush_handles(device);

    ret = ptp_object_want(params, fileid, PTPOBJECT_OBJECTINFO_LOADED | PTPOBJECT_MTPPROPLIST_LOADED, &ob);
    if (ret != PTP_RC_OK)
        return NULL;

    return obj2file(device, ob);
}

/**
 * This gets a file off the device to a local file identified
 * by a filename.
 * @param device a pointer to the device to get the track from.
 * @param id the file ID of the file to retrieve.
 * @param path a filename to use for the retrieved file.
 * @param callback a progress indicator function or NULL to ignore.
 * @param data a user-defined pointer that is passed along to
 *             the <code>progress</code> function in order to
 *             pass along some user defined data to the progress
 *             updates. If not used, set this to NULL.
 * @return 0 if the transfer was successful, any other value means
 *           failure.
 * @see LIBMTP_Get_File_To_File_Descriptor()
 */
int LIBMTP_Get_File_To_File(LIBMTP_mtpdevice_t *device,
                            uint32_t const id,
                            char const * const path,
                            LIBMTP_progressfunc_t const callback,
                            void const * const data)
{
    int fd = -1;
    int ret;

    /* Sanity check */
    if (path == NULL) {
        add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Get_File_To_File(): Bad arguments, path was NULL.");
        return -1;
    }

    /* Open file */
#ifdef __WIN32__
#ifdef USE_WINDOWS_IO_H
    fd = _open(path, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, _S_IREAD);
#else
    fd = open(path, O_RDWR|O_CREAT|O_TRUNC|O_BINARY,S_IRWXU);
#endif
#else
    fd = open(path, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRGRP);
#endif
    if (fd == -1) {
        add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Get_File_To_File(): Could not create file.");
        return -1;
    }

    ret = LIBMTP_Get_File_To_File_Descriptor(device, id, fd, callback, data);

    /* Close file */
#ifdef USE_WINDOWS_IO_H
    _close(fd);
#else
    close(fd);
#endif

    /* Delete partial file. */
    if (ret == -1)
        unlink(path);

    return ret;
}

/**
 * This gets a file off the device to a file identified
 * by a file descriptor.
 *
 * This function can potentially be used for streaming
 * files off the device for playback or broadcast for example,
 * by downloading the file into a stream sink e.g. a socket.
 *
 * @param device a pointer to the device to get the file from.
 * @param id the file ID of the file to retrieve.
 * @param fd a local file descriptor to write the file to.
 * @param callback a progress indicator function or NULL to ignore.
 * @param data a user-defined pointer that is passed along to
 *             the <code>progress</code> function in order to
 *             pass along some user defined data to the progress
 *             updates. If not used, set this to NULL.
 * @return 0 if the transfer was successful, any other value means
 *           failure.
 * @see LIBMTP_Get_File_To_File()
 */
int LIBMTP_Get_File_To_File_Descriptor(LIBMTP_mtpdevice_t *device,
                                       uint32_t const id,
                                       int const fd,
                                       LIBMTP_progressfunc_t const callback,
                                       void const * const data)
{
    uint16_t ret;
    PTPParams *params = (PTPParams *) device->params;
    PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;
    PTPObject *ob;

    ret = ptp_object_want (params, id, PTPOBJECT_OBJECTINFO_LOADED, &ob);
    if (ret != PTP_RC_OK) {
        add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Get_File_To_File_Descriptor(): Could not get object info.");
        return -1;
    }
    if (ob->oi.ObjectFormat == PTP_OFC_Association) {
        add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Get_File_To_File_Descriptor(): Bad object format.");
        return -1;
    }

    /* Callbacks */
    ptp_usb->callback_active = 1;
    /* Request length, one parameter */
    ptp_usb->current_transfer_total = ob->oi.ObjectCompressedSize + PTP_USB_BULK_HDR_LEN + sizeof(uint32_t);
    ptp_usb->current_transfer_complete = 0;
    ptp_usb->current_transfer_callback = callback;
    ptp_usb->current_transfer_callback_data = data;

    ret = ptp_getobject_tofd(params, id, fd);

    ptp_usb->callback_active = 0;
    ptp_usb->current_transfer_callback = NULL;
    ptp_usb->current_transfer_callback_data = NULL;

    if (ret == PTP_ERROR_CANCEL) {
        add_error_to_errorstack(device, LIBMTP_ERROR_CANCELLED, "LIBMTP_Get_File_From_File_Descriptor(): Cancelled transfer.");
        return -1;
    }
    if (ret != PTP_RC_OK) {
        add_ptp_error_to_errorstack(device, ret, "LIBMTP_Get_File_To_File_Descriptor(): Could not get file from device.");
        return -1;
    }

    return 0;
}

/**
 * This gets a file off the device and calls put_func
 * with chunks of data
 *
 * @param device a pointer to the device to get the file from.
 * @param id the file ID of the file to retrieve.
 * @param put_func the function to call when we have data.
 * @param priv the user-defined pointer that is passed to
 *             <code>put_func</code>.
 * @param callback a progress indicator function or NULL to ignore.
 * @param data a user-defined pointer that is passed along to
 *             the <code>progress</code> function in order to
 *             pass along some user defined data to the progress
 *             updates. If not used, set this to NULL.
 * @return 0 if the transfer was successful, any other value means
 *           failure.
 */
int LIBMTP_Get_File_To_Handler(LIBMTP_mtpdevice_t *device,
                               uint32_t const id,
                               MTPDataPutFunc put_func,
                               void * priv,
                               LIBMTP_progressfunc_t const callback,
                               void const * const data)
{
    PTPObject *ob;
    uint16_t ret;
    PTPParams *params = (PTPParams *) device->params;
    PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;

    ret = ptp_object_want (params, id, PTPOBJECT_OBJECTINFO_LOADED, &ob);
    if (ret != PTP_RC_OK) {
        add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Get_File_To_File_Descriptor(): Could not get object info.");
        return -1;
    }
    if (ob->oi.ObjectFormat == PTP_OFC_Association) {
        add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Get_File_To_File_Descriptor(): Bad object format.");
        return -1;
    }

    /* Callbacks */
    ptp_usb->callback_active = 1;
    /* Request length, one parameter */
    ptp_usb->current_transfer_total = ob->oi.ObjectCompressedSize + PTP_USB_BULK_HDR_LEN + sizeof(uint32_t);
    ptp_usb->current_transfer_complete = 0;
    ptp_usb->current_transfer_callback = callback;
    ptp_usb->current_transfer_callback_data = data;

    MTPDataHandler mtp_handler;
    mtp_handler.getfunc = NULL;
    mtp_handler.putfunc = put_func;
    mtp_handler.priv = priv;

    PTPDataHandler handler;
    handler.getfunc = NULL;
    handler.putfunc = put_func_wrapper;
    handler.priv = &mtp_handler;

    ret = ptp_getobject_to_handler(params, id, &handler);

    ptp_usb->callback_active = 0;
    ptp_usb->current_transfer_callback = NULL;
    ptp_usb->current_transfer_callback_data = NULL;

    if (ret == PTP_ERROR_CANCEL) {
        add_error_to_errorstack(device, LIBMTP_ERROR_CANCELLED, "LIBMTP_Get_File_From_File_Descriptor(): Cancelled transfer.");
        return -1;
    }
    if (ret != PTP_RC_OK) {
        add_ptp_error_to_errorstack(device, ret, "LIBMTP_Get_File_To_File_Descriptor(): Could not get file from device.");
        return -1;
    }

    return 0;
}

/**
 * This function sends a local file to an MTP device.
 * A filename and a set of metadata must be
 * given as input.
 * @param device a pointer to the device to send the track to.
 * @param path the filename of a local file which will be sent.
 * @param filedata a file metadata set to be written along with the file.
 *        After this call the field <code>filedata-&gt;item_id</code>
 *        will contain the new file ID. Other fields such
 *        as the <code>filedata-&gt;filename</code>, <code>filedata-&gt;parent_id</code>
 *        or <code>filedata-&gt;storage_id</code> may also change during this
 *        operation due to device restrictions, so do not rely on the
 *        contents of this struct to be preserved in any way.
 *        <ul>
 *        <li><code>filedata-&gt;parent_id</code> should be set to the parent
 *        (e.g. folder) to store this file in. If this is 0,
 *        the file will be stored in the root folder.
 *        <li><code>filedata-&gt;storage_id</code> should be set to the
 *        desired storage (e.g. memory card or whatever your device
 *        presents) to store this file in. Setting this to 0 will store
 *        the file on the primary storage.
 *        </ul>
 * @param callback a progress indicator function or NULL to ignore.
 * @param data a user-defined pointer that is passed along to
 *             the <code>progress</code> function in order to
 *             pass along some user defined data to the progress
 *             updates. If not used, set this to NULL.
 * @return 0 if the transfer was successful, any other value means
 *           failure.
 * @see LIBMTP_Send_File_From_File_Descriptor()
 * @see LIBMTP_Delete_Object()
 */
int LIBMTP_Send_File_From_File(LIBMTP_mtpdevice_t *device,
                               char const * const path,
                               LIBMTP_file_t * const filedata,
                               LIBMTP_progressfunc_t const callback,
                               void const * const data)
{
    int fd;
    int ret;

    /* Sanity check */
    if (path == NULL) {
        add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Send_File_From_File(): Bad arguments, path was NULL.");
        return -1;
    }

    /* Open file */
#ifdef __WIN32__
#ifdef USE_WINDOWS_IO_H
    fd = _open(path, O_RDONLY|O_BINARY));
#else
    fd = open(path, O_RDONLY|O_BINARY);
#endif
#else
    fd = open(path, O_RDONLY);
#endif
    if (fd == -1) {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Send_File_From_File(): Could not open source file.");
        return -1;
    }

    ret = LIBMTP_Send_File_From_File_Descriptor(device, fd, filedata, callback, data);

    /* Close file. */
#ifdef USE_WINDOWS_IO_H
    _close(fd);
#else
    close(fd);
#endif

    return ret;
}

/**
 * This function sends a generic file from a file descriptor to an
 * MTP device. A filename and a set of metadata must be
 * given as input.
 *
 * This can potentially be used for sending in a stream of unknown
 * length. Send music files with
 * <code>LIBMTP_Send_Track_From_File_Descriptor()</code>
 *
 * @param device a pointer to the device to send the file to.
 * @param fd the filedescriptor for a local file which will be sent.
 * @param filedata a file metadata set to be written along with the file.
 *        After this call the field <code>filedata-&gt;item_id</code>
 *        will contain the new file ID. Other fields such
 *        as the <code>filedata-&gt;filename</code>, <code>filedata-&gt;parent_id</code>
 *        or <code>filedata-&gt;storage_id</code> may also change during this
 *        operation due to device restrictions, so do not rely on the
 *        contents of this struct to be preserved in any way.
 *        <ul>
 *        <li><code>filedata-&gt;parent_id</code> should be set to the parent
 *        (e.g. folder) to store this file in. If this is 0,
 *        the file will be stored in the root folder.
 *        <li><code>filedata-&gt;storage_id</code> should be set to the
 *        desired storage (e.g. memory card or whatever your device
 *        presents) to store this file in. Setting this to 0 will store
 *        the file on the primary storage.
 *        </ul>
 * @param callback a progress indicator function or NULL to ignore.
 * @param data a user-defined pointer that is passed along to
 *             the <code>progress</code> function in order to
 *             pass along some user defined data to the progress
 *             updates. If not used, set this to NULL.
 * @return 0 if the transfer was successful, any other value means
 *           failure.
 * @see LIBMTP_Send_File_From_File()
 * @see LIBMTP_Send_Track_From_File_Descriptor()
 * @see LIBMTP_Delete_Object()
 */
int LIBMTP_Send_File_From_File_Descriptor(LIBMTP_mtpdevice_t *device,
        int const fd,
        LIBMTP_file_t * const filedata,
        LIBMTP_progressfunc_t const callback,
        void const * const data)
{
    uint16_t ret;
    PTPParams *params = (PTPParams *) device->params;
    PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;
    LIBMTP_file_t *newfilemeta;
    int oldtimeout;
    int timeout;

    if (send_file_object_info(device, filedata))
        /* no need to output an error since send_file_object_info will already have done so */
        return -1;

    /* Callbacks */
    ptp_usb->callback_active = 1;
    /* The callback will deactivate itself after this amount of data has been sent
     * One BULK header for the request, one for the data phase. No parameters to the request. */
    ptp_usb->current_transfer_total = filedata->filesize+PTP_USB_BULK_HDR_LEN*2;
    ptp_usb->current_transfer_complete = 0;
    ptp_usb->current_transfer_callback = callback;
    ptp_usb->current_transfer_callback_data = data;

    /*
     * We might need to increase the timeout here, files can be pretty
     * large. Take the default timeout and add the calculated time for
     * this transfer
     */
    get_usb_device_timeout(ptp_usb, &oldtimeout);
    timeout = oldtimeout + (ptp_usb->current_transfer_total / guess_usb_speed(ptp_usb)) * 1000;
    set_usb_device_timeout(ptp_usb, timeout);

    ret = ptp_sendobject_fromfd(params, fd, filedata->filesize);

    ptp_usb->callback_active = 0;
    ptp_usb->current_transfer_callback = NULL;
    ptp_usb->current_transfer_callback_data = NULL;
    set_usb_device_timeout(ptp_usb, oldtimeout);

    if (ret == PTP_ERROR_CANCEL) {
        add_error_to_errorstack(device, LIBMTP_ERROR_CANCELLED,
                                "LIBMTP_Send_File_From_File_Descriptor(): Cancelled transfer.");
        return -1;
    }
    if (ret != PTP_RC_OK) {
        add_ptp_error_to_errorstack(device, ret,
                                    "LIBMTP_Send_File_From_File_Descriptor(): Could not send object.");
        return -1;
    }

    add_object_to_cache(device, filedata->item_id);

    /*
     * Get the device-assigned parent_id from the cache.
     * The operation that adds it to the cache will
     * look it up from the device, so we get the new
     * parent_id from the cache.
     */
    newfilemeta = LIBMTP_Get_Filemetadata(device, filedata->item_id);
    if (newfilemeta != NULL) {
        filedata->parent_id = newfilemeta->parent_id;
        filedata->storage_id = newfilemeta->storage_id;
        LIBMTP_destroy_file_t(newfilemeta);
    } else {
        add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL,
                                "LIBMTP_Send_File_From_File_Descriptor(): Could not retrieve updated metadata.");
        return -1;
    }

    return 0;
}

/**
 * This function sends a generic file from a handler function to an
 * MTP device. A filename and a set of metadata must be
 * given as input.
 *
 * This can potentially be used for sending in a stream of unknown
 * length. Send music files with
 * <code>LIBMTP_Send_Track_From_Handler()</code>
 *
 * @param device a pointer to the device to send the file to.
 * @param get_func the function to call to get data to write
 * @param priv a user-defined pointer that is passed along to
 *        <code>get_func</code>. If not used, this is set to NULL.
 * @param filedata a file metadata set to be written along with the file.
 *        After this call the field <code>filedata-&gt;item_id</code>
 *        will contain the new file ID. Other fields such
 *        as the <code>filedata-&gt;filename</code>, <code>filedata-&gt;parent_id</code>
 *        or <code>filedata-&gt;storage_id</code> may also change during this
 *        operation due to device restrictions, so do not rely on the
 *        contents of this struct to be preserved in any way.
 *        <ul>
 *        <li><code>filedata-&gt;parent_id</code> should be set to the parent
 *        (e.g. folder) to store this file in. If this is 0,
 *        the file will be stored in the root folder.
 *        <li><code>filedata-&gt;storage_id</code> should be set to the
 *        desired storage (e.g. memory card or whatever your device
 *        presents) to store this file in. Setting this to 0 will store
 *        the file on the primary storage.
 *        </ul>
 * @param callback a progress indicator function or NULL to ignore.
 * @param data a user-defined pointer that is passed along to
 *             the <code>progress</code> function in order to
 *             pass along some user defined data to the progress
 *             updates. If not used, set this to NULL.
 * @return 0 if the transfer was successful, any other value means
 *           failure.
 * @see LIBMTP_Send_File_From_File()
 * @see LIBMTP_Send_Track_From_File_Descriptor()
 * @see LIBMTP_Delete_Object()
 */
int LIBMTP_Send_File_From_Handler(LIBMTP_mtpdevice_t *device,
                                  MTPDataGetFunc get_func,
                                  void * priv,
                                  LIBMTP_file_t * const filedata,
                                  LIBMTP_progressfunc_t const callback,
                                  void const * const data)
{
    uint16_t ret;
    PTPParams *params = (PTPParams *) device->params;
    PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;
    LIBMTP_file_t *newfilemeta;

    if (send_file_object_info(device, filedata))
        return -1;

    /* Callbacks */
    ptp_usb->callback_active = 1;
    /* The callback will deactivate itself after this amount of data has been sent
     * One BULK header for the request, one for the data phase. No parameters to the request. */
    ptp_usb->current_transfer_total = filedata->filesize+PTP_USB_BULK_HDR_LEN*2;
    ptp_usb->current_transfer_complete = 0;
    ptp_usb->current_transfer_callback = callback;
    ptp_usb->current_transfer_callback_data = data;

    MTPDataHandler mtp_handler;
    mtp_handler.getfunc = get_func;
    mtp_handler.putfunc = NULL;
    mtp_handler.priv = priv;

    PTPDataHandler handler;
    handler.getfunc = get_func_wrapper;
    handler.putfunc = NULL;
    handler.priv = &mtp_handler;

    ret = ptp_sendobject_from_handler(params, &handler, filedata->filesize);

    ptp_usb->callback_active = 0;
    ptp_usb->current_transfer_callback = NULL;
    ptp_usb->current_transfer_callback_data = NULL;

    if (ret == PTP_ERROR_CANCEL) {
        add_error_to_errorstack(device, LIBMTP_ERROR_CANCELLED,
                                "LIBMTP_Send_File_From_Handler(): Cancelled transfer.");
        return -1;
    }
    if (ret != PTP_RC_OK) {
        add_ptp_error_to_errorstack(device, ret,
                                    "LIBMTP_Send_File_From_Handler(): Could not send object.");
        return -1;
    }

    add_object_to_cache(device, filedata->item_id);

    /*
     * Get the device-assined parent_id from the cache.
     * The operation that adds it to the cache will
     * look it up from the device, so we get the new
     * parent_id from the cache.
     */
    newfilemeta = LIBMTP_Get_Filemetadata(device, filedata->item_id);
    if (newfilemeta != NULL) {
        filedata->parent_id = newfilemeta->parent_id;
        filedata->storage_id = newfilemeta->storage_id;
        LIBMTP_destroy_file_t(newfilemeta);
    } else {
        add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL,
                                "LIBMTP_Send_File_From_Handler(): Could not retrieve updated metadata.");
        return -1;
    }

    return 0;
}

/**
 * This function renames a single file.
 * This simply means that the PTP_OPC_ObjectFileName property
 * is updated, if this is supported by the device.
 *
 * @param device a pointer to the device that contains the file.
 * @param file the file metadata of the file to rename.
 *        On success, the filename member is updated. Be aware, that
 *        this name can be different than newname depending of device restrictions.
 * @param newname the new filename for this object.
 * @return 0 on success, any other value means failure.
 */
int LIBMTP_Set_File_Name(LIBMTP_mtpdevice_t *device,
                         LIBMTP_file_t *file,
                         const char *newname)
{
    int         ret;

    ret = set_object_filename(device, file->item_id, map_libmtp_type_to_ptp_type(file->filetype), &newname);

    if (ret != 0)
        return ret;

    free(file->filename);
    file->filename = strdup(newname);
    return ret;
}

/**
 * This creates a new sample data metadata structure and allocates memory
 * for it. Notice that if you add strings to this structure they
 * will be freed by the corresponding <code>LIBMTP_destroy_sampledata_t</code>
 * operation later, so be careful of using strdup() when assigning
 * strings.
 *
 * @return a pointer to the newly allocated metadata structure.
 * @see LIBMTP_destroy_sampledata_t()
 */
LIBMTP_filesampledata_t *LIBMTP_new_filesampledata_t(void)
{
    LIBMTP_filesampledata_t *sample = (LIBMTP_filesampledata_t *) malloc(sizeof(LIBMTP_filesampledata_t));
    if (new == NULL)
        return NULL;
    sample->height=0;
    sample->width = 0;
    sample->data = NULL;
    sample->duration = 0;
    sample->size = 0;
    return sample;
}

/**
 * This destroys a file sample metadata type.
 * @param sample the file sample metadata to be destroyed.
 */
void LIBMTP_destroy_filesampledata_t(LIBMTP_filesampledata_t * sample)
{
    if (sample == NULL)
        return;
    free(sample->data);
    free(sample);
}

/**
 * This routine figures out whether a certain filetype supports
 * representative samples (small thumbnail images) or not. This
 * typically applies to JPEG files, MP3 files and Album abstract
 * playlists, but in theory any filetype could support representative
 * samples.
 * @param device a pointer to the device which is to be examined.
 * @param filetype the fileype to examine, and return the representative sample
 *        properties for.
 * @param sample this will contain a new sample type with the fields
 *        filled in with suitable default values. For example, the
 *        supported sample type will be set, the supported height and
 *        width will be set to max values if it is an image sample,
 *        and duration will also be given some suitable default value
 *        which should not be exceeded on audio samples. If the
 *        device does not support samples for this filetype, this
 *        pointer will be NULL. If it is not NULL, the user must
 *        destroy this struct with <code>LIBMTP_destroy_filesampledata_t()</code>
 *        after use.
 * @return 0 on success, any other value means failure.
 * @see LIBMTP_Send_Representative_Sample()
 * @see LIBMTP_Create_New_Album()
 */
int LIBMTP_Get_Representative_Sample_Format(LIBMTP_mtpdevice_t *device,
                                            LIBMTP_filetype_t const filetype,
                                            LIBMTP_filesampledata_t ** sample)
{
    uint16_t ret;
    PTPParams *params = (PTPParams *) device->params;
    uint16_t *props = NULL;
    uint32_t propcnt = 0;
    int i;
    /* TODO: Get rid of these when we can properly query the device. */
    int support_data = 0;
    int support_format = 0;
    int support_height = 0;
    int support_width = 0;
    int support_duration = 0;
    int support_size = 0;

    PTPObjectPropDesc opd_height;
    PTPObjectPropDesc opd_width;
    PTPObjectPropDesc opd_format;
    PTPObjectPropDesc opd_duration;
    PTPObjectPropDesc opd_size;

    /* Default to no type supported. */
    *sample = NULL;

    ret = ptp_mtp_getobjectpropssupported(params, map_libmtp_type_to_ptp_type(filetype), &propcnt, &props);
    if (ret != PTP_RC_OK) {
        add_ptp_error_to_errorstack(device, ret, "LIBMTP_Get_Representative_Sample_Format(): could not get object properties.");
        return -1;
    }
    /*
     * TODO: when walking through these object properties, make calls to
     * a new function in ptp.h/ptp.c that can send the command
     * PTP_OC_MTP_GetObjectPropDesc to get max/min values of the properties
     * supported.
     */
    for (i = 0; i < propcnt; i++) {
        switch(props[i]) {
        case PTP_OPC_RepresentativeSampleData:
            support_data = 1;
            break;
        case PTP_OPC_RepresentativeSampleFormat:
            support_format = 1;
            break;
        case PTP_OPC_RepresentativeSampleSize:
            support_size = 1;
            break;
        case PTP_OPC_RepresentativeSampleHeight:
            support_height = 1;
            break;
        case PTP_OPC_RepresentativeSampleWidth:
            support_width = 1;
            break;
        case PTP_OPC_RepresentativeSampleDuration:
            support_duration = 1;
            break;
        default:
            break;
        }
    }
    free(props);

    if (support_data && support_format && support_height && support_width && !support_duration) {
        /* Something that supports height and width and not duration is likely to be JPEG */
        LIBMTP_filesampledata_t *retsam = LIBMTP_new_filesampledata_t();
        /*
         * Populate the sample format with the first supported format
         *
         * TODO: figure out how to pass back more than one format if more are
         * supported by the device.
         */
        ptp_mtp_getobjectpropdesc (params, PTP_OPC_RepresentativeSampleFormat, map_libmtp_type_to_ptp_type(filetype), &opd_format);
        retsam->filetype = map_ptp_type_to_libmtp_type(opd_format.FORM.Enum.SupportedValue[0].u16);
        ptp_free_objectpropdesc(&opd_format);
        /* Populate the maximum image height */
        ptp_mtp_getobjectpropdesc (params, PTP_OPC_RepresentativeSampleWidth, map_libmtp_type_to_ptp_type(filetype), &opd_width);
        retsam->width = opd_width.FORM.Range.MaximumValue.u32;
        ptp_free_objectpropdesc(&opd_width);
        /* Populate the maximum image width */
        ptp_mtp_getobjectpropdesc (params, PTP_OPC_RepresentativeSampleHeight, map_libmtp_type_to_ptp_type(filetype), &opd_height);
        retsam->height = opd_height.FORM.Range.MaximumValue.u32;
        ptp_free_objectpropdesc(&opd_height);
        /* Populate the maximum size */
        if (support_size) {
            ptp_mtp_getobjectpropdesc (params, PTP_OPC_RepresentativeSampleSize, map_libmtp_type_to_ptp_type(filetype), &opd_size);
            retsam->size = opd_size.FORM.Range.MaximumValue.u32;
            ptp_free_objectpropdesc(&opd_size);
        }
        *sample = retsam;
    } else if (support_data && support_format && !support_height && !support_width && support_duration) {
        /* Another qualified guess */
        LIBMTP_filesampledata_t *retsam = LIBMTP_new_filesampledata_t();
        /*
         * Populate the sample format with the first supported format
         *
         * TODO: figure out how to pass back more than one format if more are
         * supported by the device.
         */
        ptp_mtp_getobjectpropdesc (params, PTP_OPC_RepresentativeSampleFormat, map_libmtp_type_to_ptp_type(filetype), &opd_format);
        retsam->filetype = map_ptp_type_to_libmtp_type(opd_format.FORM.Enum.SupportedValue[0].u16);
        ptp_free_objectpropdesc(&opd_format);
        /* Populate the maximum duration */
        ptp_mtp_getobjectpropdesc (params, PTP_OPC_RepresentativeSampleDuration, map_libmtp_type_to_ptp_type(filetype), &opd_duration);
        retsam->duration = opd_duration.FORM.Range.MaximumValue.u32;
        ptp_free_objectpropdesc(&opd_duration);
        /* Populate the maximum size */
        if (support_size) {
            ptp_mtp_getobjectpropdesc (params, PTP_OPC_RepresentativeSampleSize, map_libmtp_type_to_ptp_type(filetype), &opd_size);
            retsam->size = opd_size.FORM.Range.MaximumValue.u32;
            ptp_free_objectpropdesc(&opd_size);
        }
        *sample = retsam;
    }
    return 0;
}

/**
 * This routine sends representative sample data for an object.
 * This uses the RepresentativeSampleData property of the album,
 * if the device supports it. The data should be of a format acceptable
 * to the player (for iRiver and Creative, this seems to be JPEG) and
 * must not be too large. (for a Creative, max seems to be about 20KB.)
 * Check by calling LIBMTP_Get_Representative_Sample_Format() to get
 * maximum size, dimensions, etc..
 * @param device a pointer to the device which the object is on.
 * @param id unique id of the object to set artwork for.
 * @param pointer to LIBMTP_filesampledata_t struct containing data
 * @return 0 on success, any other value means failure.
 * @see LIBMTP_Get_Representative_Sample()
 * @see LIBMTP_Get_Representative_Sample_Format()
 * @see LIBMTP_Create_New_Album()
 */
int LIBMTP_Send_Representative_Sample(LIBMTP_mtpdevice_t *device,
                                      uint32_t const id,
                                      LIBMTP_filesampledata_t *sampledata)
{
    uint16_t ret;
    PTPParams *params = (PTPParams *) device->params;
    PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;
    PTPPropertyValue propval;
    PTPObject *ob;
    uint32_t i;
    uint16_t *props = NULL;
    uint32_t propcnt = 0;
    int supported = 0;

    /* get the file format for the object we're going to send representative data for */
    ret = ptp_object_want (params, id, PTPOBJECT_OBJECTINFO_LOADED, &ob);
    if (ret != PTP_RC_OK) {
        add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Send_Representative_Sample(): could not get object info.");
        return -1;
    }

    /* check that we can send representative sample data for this object format */
    ret = ptp_mtp_getobjectpropssupported(params, ob->oi.ObjectFormat, &propcnt, &props);
    if (ret != PTP_RC_OK) {
        add_ptp_error_to_errorstack(device, ret, "LIBMTP_Send_Representative_Sample(): could not get object properties.");
        return -1;
    }

    for (i = 0; i < propcnt; i++) {
        if (props[i] == PTP_OPC_RepresentativeSampleData) {
            supported = 1;
            break;
        }
    }
    if (!supported) {
        free(props);
        add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Send_Representative_Sample(): object type doesn't support RepresentativeSampleData.");
        return -1;
    }
    free(props);

    /* Go ahead and send the data */
    propval.a.count = sampledata->size;
    propval.a.v = malloc(sizeof(PTPPropertyValue) * sampledata->size);
    for (i = 0; i < sampledata->size; i++)
        propval.a.v[i].u8 = sampledata->data[i];

    ret = ptp_mtp_setobjectpropvalue(params,id,PTP_OPC_RepresentativeSampleData, &propval,PTP_DTC_AUINT8);
    if (ret != PTP_RC_OK) {
        add_ptp_error_to_errorstack(device, ret, "LIBMTP_Send_Representative_Sample(): could not send sample data.");
        free(propval.a.v);
        return -1;
    }
    free(propval.a.v);

    /* Set the height and width if the sample is an image, otherwise just
     * set the duration and size */
    switch(sampledata->filetype) {
    case LIBMTP_FILETYPE_JPEG:
    case LIBMTP_FILETYPE_JFIF:
    case LIBMTP_FILETYPE_TIFF:
    case LIBMTP_FILETYPE_BMP:
    case LIBMTP_FILETYPE_GIF:
    case LIBMTP_FILETYPE_PICT:
    case LIBMTP_FILETYPE_PNG:
        if (FLAG_BROKEN_SET_SAMPLE_DIMENSIONS(ptp_usb))
            break;
        /* For images, set the height and width */
        set_object_u32(device, id, PTP_OPC_RepresentativeSampleHeight, sampledata->height);
        set_object_u32(device, id, PTP_OPC_RepresentativeSampleWidth, sampledata->width);
        break;
    default:
        /* For anything not an image, set the duration and size */
        set_object_u32(device, id, PTP_OPC_RepresentativeSampleDuration, sampledata->duration);
        set_object_u32(device, id, PTP_OPC_RepresentativeSampleSize, sampledata->size);
        break;
    }

    return 0;
}

/**
 * This routine gets representative sample data for an object.
 * This uses the RepresentativeSampleData property of the album,
 * if the device supports it.
 * @param device a pointer to the device which the object is on.
 * @param id unique id of the object to get data for.
 * @param pointer to LIBMTP_filesampledata_t struct to receive data
 * @return 0 on success, any other value means failure.
 * @see LIBMTP_Send_Representative_Sample()
 * @see LIBMTP_Get_Representative_Sample_Format()
 * @see LIBMTP_Create_New_Album()
 */
int LIBMTP_Get_Representative_Sample(LIBMTP_mtpdevice_t *device,
                                     uint32_t const id,
                                     LIBMTP_filesampledata_t *sampledata)
{
    uint16_t ret;
    PTPParams *params = (PTPParams *) device->params;
    PTPPropertyValue propval;
    PTPObject *ob;
    uint32_t i;
    uint16_t *props = NULL;
    uint32_t propcnt = 0;
    int supported = 0;

    /* get the file format for the object we're going to send representative data for */
    ret = ptp_object_want (params, id, PTPOBJECT_OBJECTINFO_LOADED, &ob);
    if (ret != PTP_RC_OK) {
        add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Get_Representative_Sample(): could not get object info.");
        return -1;
    }

    /* check that we can store representative sample data for this object format */
    ret = ptp_mtp_getobjectpropssupported(params, ob->oi.ObjectFormat, &propcnt, &props);
    if (ret != PTP_RC_OK) {
        add_ptp_error_to_errorstack(device, ret, "LIBMTP_Get_Representative_Sample(): could not get object properties.");
        return -1;
    }

    for (i = 0; i < propcnt; i++) {
        if (props[i] == PTP_OPC_RepresentativeSampleData) {
            supported = 1;
            break;
        }
    }
    if (!supported) {
        free(props);
        add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Get_Representative_Sample(): object type doesn't support RepresentativeSampleData.");
        return -1;
    }
    free(props);

    /* Get the data */
    ret = ptp_mtp_getobjectpropvalue(params,id,PTP_OPC_RepresentativeSampleData, &propval,PTP_DTC_AUINT8);
    if (ret != PTP_RC_OK) {
        add_ptp_error_to_errorstack(device, ret, "LIBMTP_Get_Representative_Sample(): could not get sample data.");
        return -1;
    }

    /* Store it */
    sampledata->size = propval.a.count;
    sampledata->data = malloc(sizeof(PTPPropertyValue) * propval.a.count);
    for (i = 0; i < propval.a.count; i++)
        sampledata->data[i] = propval.a.v[i].u8;
    free(propval.a.v);

    /* Get the other properties */
    sampledata->width = get_u32_from_object(device, id, PTP_OPC_RepresentativeSampleWidth, 0);
    sampledata->height = get_u32_from_object(device, id, PTP_OPC_RepresentativeSampleHeight, 0);
    sampledata->duration = get_u32_from_object(device, id, PTP_OPC_RepresentativeSampleDuration, 0);
    sampledata->filetype = map_ptp_type_to_libmtp_type(
                               get_u16_from_object(device, id, PTP_OPC_RepresentativeSampleFormat, LIBMTP_FILETYPE_UNKNOWN));

    return 0;
}

/**
 * Retrieve the thumbnail for a file.
 * @param device a pointer to the device to get the thumbnail from.
 * @param id the object ID of the file to retrieve the thumbnail for.
 * @return 0 on success, any other value means failure.
 */
int LIBMTP_Get_Thumbnail(LIBMTP_mtpdevice_t *device, uint32_t const id,
                         unsigned char **data, unsigned int *size)
{
    PTPParams *params = (PTPParams *) device->params;
    uint16_t ret;

    ret = ptp_getthumb(params, id, data, size);
    if (ret == PTP_RC_OK)
        return 0;
    return -1;
}

/**
 * This creates a new folder structure and allocates memory
 * for it. Notice that if you add strings to this structure they
 * will be freed by the corresponding <code>LIBMTP_folder_track_t</code>
 * operation later, so be careful of using strdup() when assigning
 * strings, e.g.:
 *
 * @return a pointer to the newly allocated folder structure.
 * @see LIBMTP_destroy_folder_t()
 */
LIBMTP_folder_t *LIBMTP_new_folder_t(void)
{
    LIBMTP_folder_t *folder = (LIBMTP_folder_t *) malloc(sizeof(LIBMTP_folder_t));
    if (folder == NULL)
        return NULL;
    folder->folder_id = 0;
    folder->parent_id = 0;
    folder->storage_id = 0;
    folder->name = NULL;
    folder->sibling = NULL;
    folder->child = NULL;
    return folder;
}

/**
 * This recursively deletes the memory for a folder structure.
 * This shall typically be called on a top-level folder list to
 * detsroy the entire folder tree.
 *
 * @param folder folder structure to destroy
 * @see LIBMTP_new_folder_t()
 */
void LIBMTP_destroy_folder_t(LIBMTP_folder_t *folder)
{

    if (folder == NULL)
        return;

    /* Destroy from the bottom up */
    if (folder->child != NULL)
        LIBMTP_destroy_folder_t(folder->child);

    if (folder->sibling != NULL)
        LIBMTP_destroy_folder_t(folder->sibling);

    free(folder->name);
    free(folder);
}

/**
 * This returns a list of all folders available
 * on the current MTP device.
 *
 * @param device a pointer to the device to get the folder listing for.
 * @return a list of folders
 */
LIBMTP_folder_t *LIBMTP_Get_Folder_List(LIBMTP_mtpdevice_t *device)
{
    return LIBMTP_Get_Folder_List_For_Storage(device, PTP_GOH_ALL_STORAGE);
}

/**
 * This returns a list of all folders available
 * on the current MTP device.
 *
 * @param device a pointer to the device to get the folder listing for.
 * @param storage a storage ID to get the folder list from
 * @return a list of folders
 */
LIBMTP_folder_t *LIBMTP_Get_Folder_List_For_Storage(LIBMTP_mtpdevice_t *device,
        uint32_t const storage)
{
    PTPParams *params = (PTPParams *) device->params;
    LIBMTP_folder_t head, *rv;
    int i;

    /* Get all the handles if we haven't already done that */
    if (params->nrofobjects == 0)
        flush_handles(device);

    /*
     * This creates a temporary list of the folders, this is in a
     * reverse order and uses the Folder pointers that are already
     * in the Folder structure. From this we can then build up the
     * folder hierarchy with only looking at this temporary list,
     * and removing the folders from this temporary list as we go.
     * This significantly reduces the number of operations that we
     * have to do in building the folder hierarchy. Also since the
     * temp list is in reverse order, when we prepend to the sibling
     * list things are in the same order as they were originally
     * in the handle list.
     */
    head.sibling = &head;
    head.child = &head;
    for (i = 0; i < params->nrofobjects; i++) {
        LIBMTP_folder_t *folder;
        PTPObject *ob;

        ob = &params->objects[i];
        if (ob->oi.ObjectFormat != PTP_OFC_Association)
            continue;

        if (storage != PTP_GOH_ALL_STORAGE && storage != ob->oi.StorageID)
            continue;

        /*
         * Do we know how to handle these? They are part
         * of the MTP 1.0 specification paragraph 3.6.4.
         * For AssociationDesc 0x00000001U ptp_mtp_getobjectreferences()
         * should be called on these to get the contained objects, but
         * we basically don't care. Hopefully parent_id is maintained for all
         * children, because we rely on that instead.
         */
        if (ob->oi.AssociationDesc != 0x00000000U)
            LIBMTP_INFO("MTP extended association type 0x%08x encountered\n", ob->oi.AssociationDesc);

        // Create a folder struct...
        folder = LIBMTP_new_folder_t();
        if (folder == NULL)
            /* malloc failure or so. */
            return NULL;
        folder->folder_id = ob->oid;
        folder->parent_id = ob->oi.ParentObject;
        folder->storage_id = ob->oi.StorageID;
        folder->name = (ob->oi.Filename) ? (char *)strdup(ob->oi.Filename) : NULL;

        /* pretend sibling says next, and child says prev. */
        folder->sibling = head.sibling;
        folder->child = &head;
        head.sibling->child = folder;
        head.sibling = folder;
    }

    /* We begin at the given root folder and get them all recursively */
    rv = get_subfolders_for_folder(&head, 0x00000000U);

    /* Some buggy devices may have some files in the "root folder"
     * 0xffffffff so if 0x00000000 didn't return any folders,
     * look for children of the root 0xffffffffU */
    if (rv == NULL) {
        rv = get_subfolders_for_folder(&head, 0xffffffffU);
        if (rv != NULL)
            LIBMTP_ERROR("Device have files in \"root folder\" 0xffffffffU - this is a firmware bug (but continuing)\n");
    }

    /* The temp list should be empty. Clean up any orphans just in case. */
    while (head.sibling != &head) {
        LIBMTP_folder_t *curr = head.sibling;

        LIBMTP_INFO("Orphan folder with ID: 0x%08x name: \"%s\" encountered.\n", curr->folder_id, curr->name);
        curr->sibling->child = curr->child;
        curr->child->sibling = curr->sibling;
        curr->child = NULL;
        curr->sibling = NULL;
        LIBMTP_destroy_folder_t(curr);
    }

    return rv;
}

/**
 * Helper function. Returns a folder structure for a
 * specified id.
 *
 * @param folderlist list of folders to search
 * @id id of folder to look for
 * @return a folder or NULL if not found
 */
LIBMTP_folder_t *LIBMTP_Find_Folder(LIBMTP_folder_t *folderlist, uint32_t id)
{
    LIBMTP_folder_t *ret = NULL;

    if (folderlist == NULL)
        return NULL;

    if (folderlist->folder_id == id)
        return folderlist;

    if (folderlist->sibling)
        ret = LIBMTP_Find_Folder(folderlist->sibling, id);

    if (folderlist->child && ret == NULL)
        ret = LIBMTP_Find_Folder(folderlist->child, id);

    return ret;
}

/**
 * This create a folder on the current MTP device. The PTP name
 * for a folder is "association". The PTP/MTP devices does not
 * have an internal "folder" concept really, it contains a flat
 * list of all files and some file are "associations" that other
 * files and folders may refer to as its "parent".
 *
 * @param device a pointer to the device to create the folder on.
 * @param name the name of the new folder. Note this can be modified
 *        if the device does not support all the characters in the
 *        name.
 * @param parent_id id of parent folder to add the new folder to,
 *        or 0 to put it in the root directory.
 * @param storage_id id of the storage to add this new folder to.
 *        notice that you cannot mismatch storage id and parent id:
 *        they must both be on the same storage! Pass in 0 if you
 *        want to create this folder on the default storage.
 * @return id to new folder or 0 if an error occured
 */
uint32_t LIBMTP_Create_Folder(LIBMTP_mtpdevice_t *device,
                              char const * const name,
                              uint32_t parent_id,
                              uint32_t storage_id)
{
    PTPParams *params = (PTPParams *) device->params;
    PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;
    uint32_t parenthandle = 0;
    uint32_t store;
    PTPObjectInfo new_folder;
    uint16_t ret;
    uint32_t new_id = 0;

    if (storage_id == 0)
        /* I'm just guessing that a folder may require 512 bytes */
        store = get_suggested_storage_id(device, 512, parent_id);
    else
        store = storage_id;
    parenthandle = parent_id;

    memset(&new_folder, 0, sizeof(new_folder));
    new_folder.Filename = name;
    if (FLAG_ONLY_7BIT_FILENAMES(ptp_usb))
        strip_7bit_from_utf8(new_folder.Filename);
    new_folder.ObjectCompressedSize = 0;
    new_folder.ObjectFormat = PTP_OFC_Association;
    new_folder.ProtectionStatus = PTP_PS_NoProtection;
    new_folder.AssociationType = PTP_AT_GenericFolder;
    new_folder.ParentObject = parent_id;
    new_folder.StorageID = store;

    /* Create the object */
    if (!(params->device_flags & DEVICE_FLAG_BROKEN_SEND_OBJECT_PROPLIST) &&
            ptp_operation_issupported(params,PTP_OC_MTP_SendObjectPropList)) {

        MTPProperties *props = (MTPProperties*)calloc(2,sizeof(MTPProperties));

        props[0].property = PTP_OPC_ObjectFileName;
        props[0].datatype = PTP_DTC_STR;
        props[0].propval.str = name;

        props[1].property = PTP_OPC_Name;
        props[1].datatype = PTP_DTC_STR;
        props[1].propval.str = name;

        ret = ptp_mtp_sendobjectproplist(params, &store, &parenthandle, &new_id, PTP_OFC_Association, 0, props, 1);
        free(props);
    } else
        ret = ptp_sendobjectinfo(params, &store, &parenthandle, &new_id, &new_folder);

    if (ret != PTP_RC_OK) {
        add_ptp_error_to_errorstack(device, ret, "LIBMTP_Create_Folder: Could not send object info.");
        if (ret == PTP_RC_AccessDenied)
            add_ptp_error_to_errorstack(device, ret, "ACCESS DENIED.");
        return 0;
    }
    /* NOTE: don't destroy the new_folder objectinfo, because it is statically referencing
     * several strings. */

    add_object_to_cache(device, new_id);

    return new_id;
}

/**
 * This function renames a single folder.
 * This simply means that the PTP_OPC_ObjectFileName property
 * is updated, if this is supported by the device.
 *
 * @param device a pointer to the device that contains the file.
 * @param folder the folder metadata of the folder to rename.
 *        On success, the name member is updated. Be aware, that
 *        this name can be different than newname depending of device restrictions.
 * @param newname the new name for this object.
 * @return 0 on success, any other value means failure.
 */
int LIBMTP_Set_Folder_Name(LIBMTP_mtpdevice_t *device,
                           LIBMTP_folder_t *folder,
                           const char* newname)
{
    int ret;

    ret = set_object_filename(device, folder->folder_id, PTP_OFC_Association, &newname);

    if (ret != 0)
        return ret;

    free(folder->name);
    folder->name = strdup(newname);
    return ret;
}
