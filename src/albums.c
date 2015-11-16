
#include "albums.h"

/**
 * This creates a new album metadata structure and allocates memory
 * for it. Notice that if you add strings to this structure they
 * will be freed by the corresponding <code>LIBMTP_destroy_album_t</code>
 * operation later, so be careful of using strdup() when assigning
 * strings.
 *
 * @return a pointer to the newly allocated metadata structure.
 * @see LIBMTP_destroy_album_t()
 */
LIBMTP_album_t *LIBMTP_new_album_t(void)
{
    LIBMTP_album_t *album = (LIBMTP_album_t *) malloc(sizeof(LIBMTP_album_t));
    if (album == NULL)
        return NULL;
    album->album_id = 0;
    album->parent_id = 0;
    album->storage_id = 0;
    album->name = NULL;
    album->artist = NULL;
    album->composer = NULL;
    album->genre = NULL;
    album->tracks = NULL;
    album->no_tracks = 0;
    album->next = NULL;
    return album;
}

/**
 * This recursively deletes the memory for an album structure
 *
 * @param album structure to destroy
 * @see LIBMTP_new_album_t()
 */
void LIBMTP_destroy_album_t(LIBMTP_album_t *album)
{
    if (album == NULL)
        return;
    free(album->name);
    free(album->artist);
    free(album->composer);
    free(album->genre);
    free(album->tracks);
    free(album);
}

/**
 * This function returns a list of the albums available on the
 * device.
 *
 * @param device a pointer to the device to get the album listing from.
 * @return an album list on success, else NULL. If there are no albums
 *         on the device, NULL will be returned as well.
 * @see LIBMTP_Get_Album()
 */
LIBMTP_album_t *LIBMTP_Get_Album_List(LIBMTP_mtpdevice_t *device)
{
    /* Read all storage devices */
    return LIBMTP_Get_Album_List_For_Storage(device, 0);
}

/**
 * This function returns a list of the albums available on the
 * device. You can filter on the storage ID.
 *
 * @param device a pointer to the device to get the album listing from.
 * @param storage_id ID of device storage (if null, all storages)
 *
 * @return an album list on success, else NULL. If there are no albums
 *         on the device, NULL will be returned as well.
 * @see LIBMTP_Get_Album()
 */
LIBMTP_album_t *LIBMTP_Get_Album_List_For_Storage(LIBMTP_mtpdevice_t *device, uint32_t const storage_id)
{
    PTPParams *params = (PTPParams *) device->params;
    LIBMTP_album_t *retalbums = NULL;
    LIBMTP_album_t *curalbum = NULL;
    uint32_t i;

    /* Get all the handles if we haven't already done that */
    if (params->nrofobjects == 0)
        flush_handles(device);

    for (i = 0; i < params->nrofobjects; i++) {
        LIBMTP_album_t *alb;
        PTPObject *ob;
        uint16_t ret;

        ob = &params->objects[i];

        /* Ignore stuff that isn't an album */
        if (ob->oi.ObjectFormat != PTP_OFC_MTP_AbstractAudioAlbum)
            continue;

        /* Ignore stuff that isn't into the storage device */
        if ((storage_id != 0) && (ob->oi.StorageID != storage_id ))
            continue;

        /* Allocate a new album type */
        alb = LIBMTP_new_album_t();
        alb->album_id = ob->oid;
        alb->parent_id = ob->oi.ParentObject;
        alb->storage_id = ob->oi.StorageID;

        /* Fetch supported metadata */
        get_album_metadata(device, alb);

        /* Then get the track listing for this album */
        ret = ptp_mtp_getobjectreferences(params, alb->album_id, &alb->tracks, &alb->no_tracks);
        if (ret != PTP_RC_OK) {
            add_ptp_error_to_errorstack(device, ret, "LIBMTP_Get_Album_List(): Could not get object references.");
            alb->tracks = NULL;
            alb->no_tracks = 0;
        }

        /* Add album to a list that will be returned afterwards. */
        if (retalbums == NULL) {
            retalbums = alb;
            curalbum = alb;
        } else {
            curalbum->next = alb;
            curalbum = alb;
        }

    }
    return retalbums;
}

/**
 * This function retrieves an individual album from the device.
 * @param device a pointer to the device to get the album from.
 * @param albid the unique ID of the album to retrieve.
 * @return a valid album metadata or NULL on failure.
 * @see LIBMTP_Get_Album_List()
 */
LIBMTP_album_t *LIBMTP_Get_Album(LIBMTP_mtpdevice_t *device, uint32_t const albid)
{
    PTPParams *params = (PTPParams *) device->params;
    uint16_t ret;
    PTPObject *ob;
    LIBMTP_album_t *alb;

    /* Get all the handles if we haven't already done that */
    if (params->nrofobjects == 0)
        flush_handles(device);

    ret = ptp_object_want(params, albid, PTPOBJECT_OBJECTINFO_LOADED, &ob);
    if (ret != PTP_RC_OK)
        return NULL;

    /* Ignore stuff that isn't an album */
    if (ob->oi.ObjectFormat != PTP_OFC_MTP_AbstractAudioAlbum)
        return NULL;

    /* Allocate a new album type */
    alb = LIBMTP_new_album_t();
    alb->album_id = ob->oid;
    alb->parent_id = ob->oi.ParentObject;
    alb->storage_id = ob->oi.StorageID;

    /* Fetch supported metadata */
    get_album_metadata(device, alb);

    /* Then get the track listing for this album */
    ret = ptp_mtp_getobjectreferences(params, alb->album_id, &alb->tracks, &alb->no_tracks);
    if (ret != PTP_RC_OK) {
        add_ptp_error_to_errorstack(device, ret, "LIBMTP_Get_Album: Could not get object references.");
        alb->tracks = NULL;
        alb->no_tracks = 0;
    }

    return alb;
}

/**
 * This routine creates a new album based on the metadata
 * supplied. If the <code>tracks</code> field of the metadata
 * contains a track listing, these tracks will be added to the
 * album.
 * @param device a pointer to the device to create the new album on.
 * @param metadata the metadata for the new album. If the function
 *        exits with success, the <code>album_id</code> field of this
 *        struct will contain the new ID of the album.
 *        <ul>
 *        <li><code>metadata-&gt;parent_id</code> should be set to the parent
 *        (e.g. folder) to store this track in. Since some
 *        devices are a bit picky about where files
 *        are placed, a default folder will be chosen if libmtp
 *        has detected one for the current filetype and this
 *        parameter is set to 0. If this is 0 and no default folder
 *        can be found, the file will be stored in the root folder.
 *        <li><code>metadata-&gt;storage_id</code> should be set to the
 *        desired storage (e.g. memory card or whatever your device
 *        presents) to store this track in. Setting this to 0 will store
 *        the track on the primary storage.
 *        </ul>
 * @return 0 on success, any other value means failure.
 * @see LIBMTP_Update_Album()
 * @see LIBMTP_Delete_Object()
 */
int LIBMTP_Create_New_Album(LIBMTP_mtpdevice_t *device,
                            LIBMTP_album_t * const metadata)
{
    uint32_t localph = metadata->parent_id;

    /* Use a default folder if none given */
    if (localph == 0) {
        if (device->default_album_folder != 0)
            localph = device->default_album_folder;
        else
            localph = device->default_music_folder;
    }
    metadata->parent_id = localph;

    /* Just create a new abstract album... */
    return create_new_abstract_list(device,
                                    metadata->name,
                                    metadata->artist,
                                    metadata->composer,
                                    metadata->genre,
                                    localph,
                                    metadata->storage_id,
                                    PTP_OFC_MTP_AbstractAudioAlbum,
                                    ".alb",
                                    &metadata->album_id,
                                    metadata->tracks,
                                    metadata->no_tracks);
}

/**
 * This routine updates an album based on the metadata
 * supplied. If the <code>tracks</code> field of the metadata
 * contains a track listing, these tracks will be added to the
 * album in place of those already present, i.e. the
 * previous track listing will be deleted.
 * @param device a pointer to the device to create the new album on.
 * @param metadata the metadata for the album to be updated.
 *                 notice that the field <code>album_id</code>
 *                 must contain the apropriate album ID.
 * @return 0 on success, any other value means failure.
 * @see LIBMTP_Create_New_Album()
 * @see LIBMTP_Delete_Object()
 */
int LIBMTP_Update_Album(LIBMTP_mtpdevice_t *device,
                        LIBMTP_album_t const * const metadata)
{
    return update_abstract_list(device,
                                metadata->name,
                                metadata->artist,
                                metadata->composer,
                                metadata->genre,
                                metadata->album_id,
                                PTP_OFC_MTP_AbstractAudioAlbum,
                                metadata->tracks,
                                metadata->no_tracks);
}

/**
 * This function renames a single album.
 * This simply means that the <code>PTP_OPC_ObjectFileName</code>
 * property is updated, if this is supported by the device.
 * The album filename should nominally end with an extension
 * like ".alb".
 *
 * NOTE: if you want to change the metadata the device display
 * about a playlist you must <i>not</i> use this function,
 * use <code>LIBMTP_Update_Album()</code> instead!
 *
 * @param device a pointer to the device that contains the file.
 * @param album the album metadata of the album to rename.
 *        On success, the name member is updated. Be aware, that
 *        this name can be different than newname depending of device restrictions.
 * @param newname the new name for this object.
 * @return 0 on success, any other value means failure.
 * @see LIBMTP_Update_Album()
 */
int LIBMTP_Set_Album_Name(LIBMTP_mtpdevice_t *device,
                          LIBMTP_album_t *album, const char* newname)
{
    int ret;

    ret = set_object_filename(device, album->album_id, PTP_OFC_MTP_AbstractAudioAlbum, &newname);

    if (ret != 0)
        return ret;

    free(album->name);
    album->name = strdup(newname);
    return ret;
}
