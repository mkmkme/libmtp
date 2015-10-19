/**
 * @}
 * @defgroup basic The basic device management API.
 * @{
 */

LIBMTP_error_number_t LIBMTP_Detect_Raw_Devices(LIBMTP_raw_device_t **, int *);
int LIBMTP_Check_Specific_Device(int busno, int devno);
LIBMTP_mtpdevice_t *LIBMTP_Open_Raw_Device(LIBMTP_raw_device_t *);
LIBMTP_mtpdevice_t *LIBMTP_Open_Raw_Device_Uncached(LIBMTP_raw_device_t *);
/* Begin old, legacy interface */
LIBMTP_mtpdevice_t *LIBMTP_Get_First_Device(void);
LIBMTP_error_number_t LIBMTP_Get_Connected_Devices(LIBMTP_mtpdevice_t **);
uint32_t LIBMTP_Number_Devices_In_List(LIBMTP_mtpdevice_t *);
void LIBMTP_Release_Device_List(LIBMTP_mtpdevice_t*);
/* End old, legacy interface */
void LIBMTP_Release_Device(LIBMTP_mtpdevice_t*);
void LIBMTP_Dump_Device_Info(LIBMTP_mtpdevice_t*);
int LIBMTP_Reset_Device(LIBMTP_mtpdevice_t*);
char *LIBMTP_Get_Manufacturername(LIBMTP_mtpdevice_t*);
char *LIBMTP_Get_Modelname(LIBMTP_mtpdevice_t*);
char *LIBMTP_Get_Serialnumber(LIBMTP_mtpdevice_t*);
char *LIBMTP_Get_Deviceversion(LIBMTP_mtpdevice_t*);
char *LIBMTP_Get_Friendlyname(LIBMTP_mtpdevice_t*);
int LIBMTP_Set_Friendlyname(LIBMTP_mtpdevice_t*, char const * const);
char *LIBMTP_Get_Syncpartner(LIBMTP_mtpdevice_t*);
int LIBMTP_Set_Syncpartner(LIBMTP_mtpdevice_t*, char const * const);
int LIBMTP_Get_Batterylevel(LIBMTP_mtpdevice_t *,
                            uint8_t * const,
                            uint8_t * const);
int LIBMTP_Get_Secure_Time(LIBMTP_mtpdevice_t *, char ** const);
int LIBMTP_Get_Device_Certificate(LIBMTP_mtpdevice_t *, char ** const);
int LIBMTP_Get_Supported_Filetypes(LIBMTP_mtpdevice_t *, uint16_t ** const, uint16_t * const);
int LIBMTP_Check_Capability(LIBMTP_mtpdevice_t *, LIBMTP_devicecap_t);
LIBMTP_error_t *LIBMTP_Get_Errorstack(LIBMTP_mtpdevice_t*);
void LIBMTP_Clear_Errorstack(LIBMTP_mtpdevice_t*);
void LIBMTP_Dump_Errorstack(LIBMTP_mtpdevice_t*);

/**
 * @}
 */
