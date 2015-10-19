/**
 * @}
 * @defgroup objects The object management API.
 * @{
 */
int LIBMTP_Delete_Object(LIBMTP_mtpdevice_t *device, uint32_t id);

int LIBMTP_Set_Object_Filename(LIBMTP_mtpdevice_t *device,
                               uint32_t id,
                               const char *newname);

int LIBMTP_GetPartialObject(LIBMTP_mtpdevice_t *device,
                            uint32_t const id,
                            uint64_t offset,
                            uint32_t maxbytes,
                            unsigned char **data,
                            unsigned int *size);

int LIBMTP_SendPartialObject(LIBMTP_mtpdevice_t *device,
                             uint32_t const id,
                             uint64_t offset,
                             unsigned char *data,
                             unsigned int size);

int LIBMTP_BeginEditObject(LIBMTP_mtpdevice_t *device, uint32_t const id);

int LIBMTP_EndEditObject(LIBMTP_mtpdevice_t *device, uint32_t const id);

int LIBMTP_TruncateObject(LIBMTP_mtpdevice_t *device, uint32_t const id, uint64_t offset);

