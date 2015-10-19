
/**
 * @}
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
