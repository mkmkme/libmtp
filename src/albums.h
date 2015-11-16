
typedef struct LIBMTP_album_struct LIBMTP_album_t; /**< @see LIBMTP_album_struct */

/**
 * MTP Album structure
 */
struct LIBMTP_album_struct {
    uint32_t album_id; /**< Unique playlist ID */
    uint32_t parent_id; /**< ID of parent folder */
    uint32_t storage_id; /**< ID of storage holding this album */
    char *name; /**< Name of album */
    char *artist; /**< Name of album artist */
    char *composer; /**< Name of recording composer */
    char *genre; /**< Genre of album */
    uint32_t *tracks; /**< The tracks in this album */
    uint32_t no_tracks; /**< The number of tracks in this album */
    LIBMTP_album_t *next; /**< Next album or NULL if last album */
};

/**
 * @
 * @defgroup albums The audio/video album management API.
 * @{
 */
LIBMTP_album_t *LIBMTP_new_album_t(void);
void LIBMTP_destroy_album_t(LIBMTP_album_t *);
LIBMTP_album_t *LIBMTP_Get_Album_List(LIBMTP_mtpdevice_t *);
LIBMTP_album_t *LIBMTP_Get_Album_List_For_Storage(LIBMTP_mtpdevice_t *, uint32_t const);
LIBMTP_album_t *LIBMTP_Get_Album(LIBMTP_mtpdevice_t *, uint32_t const);
int LIBMTP_Create_New_Album(LIBMTP_mtpdevice_t *, LIBMTP_album_t * const);
int LIBMTP_Update_Album(LIBMTP_mtpdevice_t *, LIBMTP_album_t const * const);
int LIBMTP_Set_Album_Name(LIBMTP_mtpdevice_t *, LIBMTP_album_t *, const char *);
