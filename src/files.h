/**
 * @}
 * @defgroup files The file management API.
 * @{
 */

/**
 * MTP file struct
 */
struct LIBMTP_file_struct {
    uint32_t item_id; /**< Unique item ID */
    uint32_t parent_id; /**< ID of parent folder */
    uint32_t storage_id; /**< ID of storage holding this file */
    char *filename; /**< Filename of this file */
    uint64_t filesize; /**< Size of file in bytes */
    time_t modificationdate; /**< Date of last alteration of the file */
    LIBMTP_filetype_t filetype; /**< Filetype used for the current file */
    LIBMTP_file_t *next; /**< Next file in list or NULL if last file */
};

/**
 * MTP Folder structure
 */
struct LIBMTP_folder_struct {
    uint32_t folder_id; /**< Unique folder ID */
    uint32_t parent_id; /**< ID of parent folder */
    uint32_t storage_id; /**< ID of storage holding this folder */
    char *name; /**< Name of folder */
    LIBMTP_folder_t *sibling; /**< Next folder at same level or NULL if no more */
    LIBMTP_folder_t *child; /**< Child folder or NULL if no children */
};


LIBMTP_file_t *LIBMTP_new_file_t(void);

void LIBMTP_destroy_file_t(LIBMTP_file_t *file);

char const * LIBMTP_Get_Filetype_Description(LIBMTP_filetype_t intype);

LIBMTP_file_t *LIBMTP_Get_Filelisting(LIBMTP_mtpdevice_t *device);

LIBMTP_file_t *LIBMTP_Get_Filelisting_With_Callback(LIBMTP_mtpdevice_t *device,
                                                    LIBMTP_progressfunc_t const callback,
                                                    void const * const data);

LIBMTP_file_t * LIBMTP_Get_Files_And_Folders(LIBMTP_mtpdevice_t *device,
        uint32_t const storage,
        uint32_t const parent);

LIBMTP_file_t *LIBMTP_Get_Filemetadata(LIBMTP_mtpdevice_t *device, uint32_t const id);

int LIBMTP_Get_File_To_File(LIBMTP_mtpdevice_t *device,
                            uint32_t const id,
                            char const * const path,
                            LIBMTP_progressfunc_t const callback,
                            void const * const data);

int LIBMTP_Get_File_To_File_Descriptor(LIBMTP_mtpdevice_t *device,
                                       uint32_t const id,
                                       int const fd,
                                       LIBMTP_progressfunc_t const callback,
                                       void const * const data);

int LIBMTP_Get_File_To_Handler(LIBMTP_mtpdevice_t *device,
                               uint32_t const id,
                               MTPDataPutFunc put_func,
                               void *priv,
                               LIBMTP_progressfunc_t const callback,
                               void const * const data);

int LIBMTP_Send_File_From_File(LIBMTP_mtpdevice_t *device,
                               char const * const path,
                               LIBMTP_file_t * const filedata,
                               LIBMTP_progressfunc_t const callback,
                               void const * const data);

int LIBMTP_Send_File_From_File_Descriptor(LIBMTP_mtpdevice_t *device,
                                          int const fd,
                                          LIBMTP_file_t * const filedata,
                                          LIBMTP_progressfunc_t const callback,
                                          void const * const data);

int LIBMTP_Send_File_From_Handler(LIBMTP_mtpdevice_t * device,
                                  MTPDataGetFunc get_func,
                                  void *priv,
                                  LIBMTP_file_t * const filedata,
                                  LIBMTP_progressfunc_t const callback,
                                  void const * const data);

int LIBMTP_Set_File_Name(LIBMTP_mtpdevice_t *device,
                         LIBMTP_file_t *file,
                         const char *newname);

LIBMTP_filesampledata_t *LIBMTP_new_filesampledata_t(void);

void LIBMTP_destroy_filesampledata_t(LIBMTP_filesampledata_t *sample);

int LIBMTP_Get_Representative_Sample_Format(LIBMTP_mtpdevice_t *device,
                                            LIBMTP_filetype_t const filetype,
                                            LIBMTP_filesampledata_t **sample);

int LIBMTP_Send_Representative_Sample(LIBMTP_mtpdevice_t *device,
                                      uint32_t const id,
                                      LIBMTP_filesampledata_t *sampledata);

int LIBMTP_Get_Representative_Sample(LIBMTP_mtpdevice_t *device,
                                     uint32_t const id,
                                     LIBMTP_filesampledata_t *sampledata);

int LIBMTP_Get_Thumbnail(LIBMTP_mtpdevice_t *device,
                         uint32_t const id,
                         unsigned char **data,
                         unsigned int *size);

/**
 * @}
 * @defgroup folders The folder management API.
 * @{
 */

LIBMTP_folder_t *LIBMTP_new_folder_t(void);

void LIBMTP_destroy_folder_t(LIBMTP_folder_t *folder);

LIBMTP_folder_t *LIBMTP_Get_Folder_List(LIBMTP_mtpdevice_t *device);

LIBMTP_folder_t *LIBMTP_Get_Folder_List_For_Storage(LIBMTP_mtpdevice_t *device,
                                                    uint32_t const id);

LIBMTP_folder_t *LIBMTP_Find_Folder(LIBMTP_folder_t *folderlist, uint32_t const id);

uint32_t LIBMTP_Create_Folder(LIBMTP_mtpdevice_t *device,
                              char const * const name,
                              uint32_t parent_id,
                              uint32_t storage_id);

int LIBMTP_Set_Folder_Name(LIBMTP_mtpdevice_t *device,
                           LIBMTP_folder_t *folder,
                           const char *newname);

/** @} */
