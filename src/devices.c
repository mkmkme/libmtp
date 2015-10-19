
/**
 * Detect the raw MTP device descriptors and return a list of
 * of the devices found.
 *
 * @param devices a pointer to a variable that will hold
 *        the list of raw devices found. This may be NULL
 *        on return if the number of detected devices is zero.
 *        The user shall simply <code>free()</code> this
 *        variable when finished with the raw devices,
 *        in order to release memory.
 * @param numdevs a pointer to an integer that will hold
 *        the number of devices in the list. This may
 *        be 0.
 * @return 0 if successful, any other value means failure.
 */
LIBMTP_error_number_t LIBMTP_Detect_Raw_Devices(LIBMTP_raw_device_t ** devices,
        int * numdevs)
{
    mtpdevice_list_t *devlist = NULL;
    mtpdevice_list_t *dev;
    LIBMTP_error_number_t ret;
    LIBMTP_raw_device_t *retdevs;
    int devs = 0;
    int i, j;

    ret = get_mtp_usb_device_list(&devlist);
    if (ret == LIBMTP_ERROR_NO_DEVICE_ATTACHED) {
        *devices = NULL;
        *numdevs = 0;
        return ret;
    } else if (ret != LIBMTP_ERROR_NONE) {
        LIBMTP_ERROR("LIBMTP PANIC: get_mtp_usb_device_list() "
                     "error code: %d on line %d\n", ret, __LINE__);
        return ret;
    }

    // Get list size
    dev = devlist;
    while (dev != NULL) {
        devs++;
        dev = dev->next;
    }
    if (devs == 0) {
        *devices = NULL;
        *numdevs = 0;
        return LIBMTP_ERROR_NONE;
    }
    // Conjure a device list
    retdevs = (LIBMTP_raw_device_t *) malloc(sizeof(LIBMTP_raw_device_t) * devs);
    if (retdevs == NULL) {
        // Out of memory
        *devices = NULL;
        *numdevs = 0;
        return LIBMTP_ERROR_MEMORY_ALLOCATION;
    }
    dev = devlist;
    i = 0;
    while (dev != NULL) {
        int device_known = 0;
        struct libusb_device_descriptor desc;

        libusb_get_device_descriptor (dev->device, &desc);
        // Assign default device info
        retdevs[i].device_entry.vendor = NULL;
        retdevs[i].device_entry.vendor_id = desc.idVendor;
        retdevs[i].device_entry.product = NULL;
        retdevs[i].device_entry.product_id = desc.idProduct;
        retdevs[i].device_entry.device_flags = 0x00000000U;
        // See if we can locate some additional vendor info and device flags
        for(j = 0; j < mtp_device_table_size; j++) {
            if(desc.idVendor == mtp_device_table[j].vendor_id &&
                    desc.idProduct == mtp_device_table[j].product_id) {
                device_known = 1;
                retdevs[i].device_entry.vendor = mtp_device_table[j].vendor;
                retdevs[i].device_entry.product = mtp_device_table[j].product;
                retdevs[i].device_entry.device_flags = mtp_device_table[j].device_flags;

                // This device is known to the developers
                LIBMTP_ERROR("Device %d (VID=%04x and PID=%04x) is a %s %s.\n",
                             i,
                             desc.idVendor,
                             desc.idProduct,
                             mtp_device_table[j].vendor,
                             mtp_device_table[j].product);
                break;
            }
        }
        if (!device_known) {
            device_unknown(i, desc.idVendor, desc.idProduct);
        }
        // Save the location on the bus
        retdevs[i].bus_location = libusb_get_bus_number (dev->device);
        retdevs[i].devnum = libusb_get_device_address (dev->device);
        i++;
        dev = dev->next;
    }
    *devices = retdevs;
    *numdevs = i;
    free_mtpdevice_list(devlist);
    return LIBMTP_ERROR_NONE;
}

/**
 * Checks if a specific device with a certain bus and device
 * number has an MTP type device descriptor.
 *
 * @param busno the bus number of the device to check
 * @param deviceno the device number of the device to check
 * @return 1 if the device is MTP else 0
 */
int LIBMTP_Check_Specific_Device(int busno, int devno)
{
    ssize_t nrofdevs;
    libusb_device **devs = NULL;
    int i;
    LIBMTP_error_number_t init_usb_ret;

    init_usb_ret = init_usb();
    if (init_usb_ret != LIBMTP_ERROR_NONE)
        return 0;

    nrofdevs = libusb_get_device_list (NULL, &devs);
    for (i = 0; i < nrofdevs ; i++ ) {

        if (libusb_get_bus_number(devs[i]) != busno)
            continue;
        if (libusb_get_device_address(devs[i]) != devno)
            continue;

        if (probe_device_descriptor(devs[i], NULL))
            return 1;
    }
    return 0;
}

LIBMTP_mtpdevice_t *LIBMTP_Open_Raw_Device(LIBMTP_raw_device_t *rawdevice)
{
    LIBMTP_mtpdevice_t *mtp_device = LIBMTP_Open_Raw_Device_Uncached(rawdevice);

    if (mtp_device == NULL)
        return NULL;

    /* Check for MTPZ devices. */
    if (use_mtpz) {
        LIBMTP_device_extension_t *tmpext = mtp_device->extensions;

        while (tmpext != NULL) {
            if (strcmp(tmpext->name, "microsoft.com/MTPZ") == 0) {
                LIBMTP_INFO("MTPZ device detected. Authenticating...\n");
                if (PTP_RC_OK == ptp_mtpz_handshake(mtp_device->params))
                    LIBMTP_INFO ("(MTPZ) Successfully authenticated with device.\n");
                else
                    LIBMTP_INFO ("(MTPZ) Failure - could not authenticate with device.\n");
                break;
            }
            tmpext = tmpext->next;
        }
    }

    // Set up this device as cached
    mtp_device->cached = 1;
    /*
     * Then get the handles and try to locate the default folders.
     * This has the desired side effect of caching all handles from
     * the device which speeds up later operations.
     */
    flush_handles(mtp_device);
    return mtp_device;
}

/**
 * This function opens a device from a raw device. It is the
 * preferred way to access devices in the new interface where
 * several devices can come and go as the library is working
 * on a certain device.
 * @param rawdevice the raw device to open a "real" device for.
 * @return an open device.
 */
LIBMTP_mtpdevice_t *LIBMTP_Open_Raw_Device_Uncached(LIBMTP_raw_device_t *rawdevice)
{
    LIBMTP_mtpdevice_t *mtp_device;
    uint8_t bs = 0;
    PTPParams *current_params;
    PTP_USB *ptp_usb;
    LIBMTP_error_number_t err;
    int i;

    /* Allocate dynamic space for our device */
    mtp_device = (LIBMTP_mtpdevice_t *) malloc(sizeof(LIBMTP_mtpdevice_t));
    /* Check if there was a memory allocation error */
    if(mtp_device == NULL) {
        /* There has been an memory allocation error. We are going to ignore this
            device and attempt to continue */

        /* TODO: This error statement could probably be a bit more robust */
        LIBMTP_ERROR("LIBMTP PANIC: connect_usb_devices encountered a memory "
                     "allocation error with device %d on bus %d, trying to continue",
                     rawdevice->devnum, rawdevice->bus_location);

        return NULL;
    }
    memset(mtp_device, 0, sizeof(LIBMTP_mtpdevice_t));
    // Non-cached by default
    mtp_device->cached = 0;

    /* Create PTP params */
    current_params = (PTPParams *) malloc(sizeof(PTPParams));
    if (current_params == NULL) {
        free(mtp_device);
        return NULL;
    }
    memset(current_params, 0, sizeof(PTPParams));
    current_params->device_flags = rawdevice->device_entry.device_flags;
    current_params->nrofobjects = 0;
    current_params->objects = NULL;
    current_params->response_packet_size = 0;
    current_params->response_packet = NULL;
    /* This will be a pointer to PTP_USB later */
    current_params->data = NULL;
    /* Set upp local debug and error functions */
    current_params->debug_func = LIBMTP_ptp_debug;
    current_params->error_func = LIBMTP_ptp_error;
    /* TODO: Will this always be little endian? */
    current_params->byteorder = PTP_DL_LE;
    current_params->cd_locale_to_ucs2 = iconv_open("UCS-2LE", "UTF-8");
    current_params->cd_ucs2_to_locale = iconv_open("UTF-8", "UCS-2LE");

    if(current_params->cd_locale_to_ucs2 == (iconv_t) -1 ||
            current_params->cd_ucs2_to_locale == (iconv_t) -1) {
        LIBMTP_ERROR("LIBMTP PANIC: Cannot open iconv() converters to/from UCS-2!\n"
                     "Too old stdlibc, glibc and libiconv?\n");
        free(current_params);
        free(mtp_device);
        return NULL;
    }
    mtp_device->params = current_params;

    /* Create usbinfo, this also opens the session */
    err = configure_usb_device(rawdevice, current_params, &mtp_device->usbinfo);
    if (err != LIBMTP_ERROR_NONE) {
        free(current_params);
        free(mtp_device);
        return NULL;
    }
    ptp_usb = (PTP_USB*) mtp_device->usbinfo;
    /* Set pointer back to params */
    ptp_usb->params = current_params;

    /* Cache the device information for later use */
    if (ptp_getdeviceinfo(current_params, &current_params->deviceinfo) != PTP_RC_OK) {
        LIBMTP_ERROR("LIBMTP PANIC: Unable to read device information on device %d on bus %d, trying to continue",
                     rawdevice->devnum, rawdevice->bus_location);

        /* Prevent memory leaks for this device */
        free(mtp_device->usbinfo);
        free(mtp_device->params);
        current_params = NULL;
        free(mtp_device);
        return NULL;
    }

    /* Check: if this is a PTP device, is it really tagged as MTP? */
    if (current_params->deviceinfo.VendorExtensionID != 0x00000006) {
        LIBMTP_ERROR("LIBMTP WARNING: no MTP vendor extension on device %d on bus %d",
                     rawdevice->devnum, rawdevice->bus_location);
        LIBMTP_ERROR("LIBMTP WARNING: VendorExtensionID: %08x",
                     current_params->deviceinfo.VendorExtensionID);
        LIBMTP_ERROR("LIBMTP WARNING: VendorExtensionDesc: %s",
                     current_params->deviceinfo.VendorExtensionDesc);
        LIBMTP_ERROR("LIBMTP WARNING: this typically means the device is PTP "
                     "(i.e. a camera) but not an MTP device at all. "
                     "Trying to continue anyway.");
    }

    parse_extension_descriptor(mtp_device, current_params->deviceinfo.VendorExtensionDesc);

    /*
     * Android has a number of bugs, force-assign these bug flags
     * if Android is encountered. Same thing for devices we detect
     * as SONY NWZ Walkmen. I have no clue what "sony.net/WMFU" means
     * I just know only NWZs have it.
     */
    {
        LIBMTP_device_extension_t *tmpext = mtp_device->extensions;
        int is_microsoft_com_wpdna = 0;
        int is_android = 0;
        int is_sony_net_wmfu = 0;
        int is_sonyericsson_com_se = 0;

        /* Loop over extensions and set flags */
        while (tmpext != NULL) {
            if (strcmp(tmpext->name, "microsoft.com/WPDNA") == 0)
                is_microsoft_com_wpdna = 1;
            else if (strcmp(tmpext->name, "android.com") == 0)
                is_android = 1;
            else if (strcmp(tmpext->name, "sony.net/WMFU") == 0)
                is_sony_net_wmfu = 1;
            else if (strcmp(tmpext->name, "sonyericsson.com/SE") == 0)
                is_sonyericsson_com_se = 1;
            tmpext = tmpext->next;
        }

        /* Check for specific stacks */
        if (is_microsoft_com_wpdna && is_sonyericsson_com_se && !is_android) {
            /*
             * The Aricent stack seems to be detected by providing WPDNA, the SonyEricsson
             * extension and NO Android extension.
             */
            ptp_usb->rawdevice.device_entry.device_flags |= DEVICE_FLAGS_ARICENT_BUGS;
            LIBMTP_INFO("Aricent MTP stack device detected, assigning default bug flags\n");
        }
        else if (is_android) {
            /*
             * If bugs are fixed in later versions, test on tmpext->major, tmpext->minor
             */
            ptp_usb->rawdevice.device_entry.device_flags |= DEVICE_FLAGS_ANDROID_BUGS;
            LIBMTP_INFO("Android device detected, assigning default bug flags\n");
        }
        else if (is_sony_net_wmfu) {
            ptp_usb->rawdevice.device_entry.device_flags |= DEVICE_FLAGS_SONY_NWZ_BUGS;
            LIBMTP_INFO("SONY NWZ device detected, assigning default bug flags\n");
        }
    }

    /*
     * If the OGG or FLAC filetypes are flagged as "unknown", check
     * if the firmware has been updated to actually support it.
     */
    if (FLAG_OGG_IS_UNKNOWN(ptp_usb)) {
        for (i = 0; i < current_params->deviceinfo.ImageFormats_len; i++) {
            if (current_params->deviceinfo.ImageFormats[i] == PTP_OFC_MTP_OGG) {
                /* This is not unknown anymore, unflag it */
                ptp_usb->rawdevice.device_entry.device_flags &= ~DEVICE_FLAG_OGG_IS_UNKNOWN;
                break;
            }
        }
    }
    if (FLAG_FLAC_IS_UNKNOWN(ptp_usb)) {
        for (i = 0; i < current_params->deviceinfo.ImageFormats_len; i++) {
            if (current_params->deviceinfo.ImageFormats[i] == PTP_OFC_MTP_FLAC) {
                /* This is not unknown anymore, unflag it */
                ptp_usb->rawdevice.device_entry.device_flags &= ~DEVICE_FLAG_FLAC_IS_UNKNOWN;
                break;
            }
        }
    }

    /* Determine if the object size supported is 32 or 64 bit wide */
    if (ptp_operation_issupported(current_params,PTP_OC_MTP_GetObjectPropsSupported)) {
        for (i = 0; i < current_params->deviceinfo.ImageFormats_len; i++) {
            PTPObjectPropDesc opd;
            uint16_t r = ptp_mtp_getobjectpropdesc(current_params, PTP_OPC_ObjectSize, current_params->deviceinfo.ImageFormats[i], &opd);
            if (r != PTP_RC_OK)
                LIBMTP_ERROR("LIBMTP PANIC: could not inspect object property descriptions!\n");
            else {
                if (opd.DataType == PTP_DTC_UINT32) {
                    if (bs == 0)
                        bs = 32;
                    else if (bs != 32) {
                        LIBMTP_ERROR("LIBMTP PANIC: different objects support different object sizes!\n");
                        bs = 0;
                        break;
                    }
                } else if (opd.DataType == PTP_DTC_UINT64) {
                    if (bs == 0)
                        bs = 64;
                    else if (bs != 64) {
                        LIBMTP_ERROR("LIBMTP PANIC: different objects support different object sizes!\n");
                        bs = 0;
                        break;
                    }
                } else {
                    // Ignore if other size.
                    LIBMTP_ERROR("LIBMTP PANIC: awkward object size data type: %04x\n", opd.DataType);
                    bs = 0;
                    break;
                }
            }
        }
    }
    if (bs == 0)
        // Could not detect object bitsize, assume 32 bits
        bs = 32;
    mtp_device->object_bitsize = bs;

    /* No Errors yet for this device */
    mtp_device->errorstack = NULL;

    /* Default Max Battery Level, we will adjust this if possible */
    mtp_device->maximum_battery_level = 100;

    /* Check if device supports reading maximum battery level */
    if (!FLAG_BROKEN_BATTERY_LEVEL(ptp_usb) && ptp_property_issupported(current_params, PTP_DPC_BatteryLevel)) {
        PTPDevicePropDesc dpd;

        /* Try to read maximum battery level */
        if (ptp_getdevicepropdesc(current_params, PTP_DPC_BatteryLevel, &dpd) != PTP_RC_OK) {
            add_error_to_errorstack(mtp_device, LIBMTP_ERROR_CONNECTING,
                                    "Unable to read Maximum Battery Level for this device even though the device supposedly "
                                    "supports this functionality");
        }

        /* TODO: is this appropriate? */
        /* If max battery level is 0 then leave the default, otherwise assign */
        if (dpd.FORM.Range.MaximumValue.u8 != 0)
            mtp_device->maximum_battery_level = dpd.FORM.Range.MaximumValue.u8;

        ptp_free_devicepropdesc(&dpd);
    }

    /* Set all default folders to 0xffffffffU (root directory) */
    mtp_device->default_music_folder = 0xffffffffU;
    mtp_device->default_playlist_folder = 0xffffffffU;
    mtp_device->default_picture_folder = 0xffffffffU;
    mtp_device->default_video_folder = 0xffffffffU;
    mtp_device->default_organizer_folder = 0xffffffffU;
    mtp_device->default_zencast_folder = 0xffffffffU;
    mtp_device->default_album_folder = 0xffffffffU;
    mtp_device->default_text_folder = 0xffffffffU;

    /* Set initial storage information */
    mtp_device->storage = NULL;
    if (LIBMTP_Get_Storage(mtp_device, LIBMTP_STORAGE_SORTBY_NOTSORTED) == -1) {
        add_error_to_errorstack(mtp_device, LIBMTP_ERROR_GENERAL, "Get Storage information failed.");
        mtp_device->storage = NULL;
    }


    return mtp_device;
}

/**
 * Get the first (as in "first in the list of") connected MTP device.
 * @return a device pointer.
 * @see LIBMTP_Get_Connected_Devices()
 */
LIBMTP_mtpdevice_t *LIBMTP_Get_First_Device(void)
{
    LIBMTP_mtpdevice_t *first_device = NULL;
    LIBMTP_raw_device_t *devices;
    int numdevs;
    LIBMTP_error_number_t ret;

    ret = LIBMTP_Detect_Raw_Devices(&devices, &numdevs);
    if (ret != LIBMTP_ERROR_NONE)
        return NULL;

    if (devices == NULL || numdevs == 0) {
        free(devices);
        return NULL;
    }

    first_device = LIBMTP_Open_Raw_Device(&devices[0]);
    free(devices);
    return first_device;
}

/**
 * Get the first connected MTP device node in the linked list of devices.
 * Currently this only provides access to USB devices
 * @param device_list A list of devices ready to be used by the caller. You
 *        need to know how many there are.
 * @return Any error information gathered from device connections
 * @see LIBMTP_Number_Devices_In_List()
 */
LIBMTP_error_number_t LIBMTP_Get_Connected_Devices(LIBMTP_mtpdevice_t **device_list)
{
    LIBMTP_raw_device_t *devices;
    int numdevs;
    LIBMTP_error_number_t ret;

    ret = LIBMTP_Detect_Raw_Devices(&devices, &numdevs);
    if (ret != LIBMTP_ERROR_NONE) {
        *device_list = NULL;
        return ret;
    }

    /* Assign linked list of devices */
    if (devices == NULL || numdevs == 0) {
        *device_list = NULL;
        free(devices);
        return LIBMTP_ERROR_NO_DEVICE_ATTACHED;
    }

    *device_list = create_usb_mtp_devices(devices, numdevs);
    free(devices);

    /* TODO: Add wifi device access here */

    /* We have found some devices but create failed */
    if (*device_list == NULL)
        return LIBMTP_ERROR_CONNECTING;

    return LIBMTP_ERROR_NONE;
}

/**
 * Get the number of devices that are available in the listed device list
 * @param device_list Pointer to a linked list of devices
 * @return Number of devices in the device list device_list
 * @see LIBMTP_Get_Connected_Devices()
 */
uint32_t LIBMTP_Number_Devices_In_List(LIBMTP_mtpdevice_t *device_list)
{
    uint32_t numdevices = 0;
    LIBMTP_mtpdevice_t *iter;
    for(iter = device_list; iter != NULL; iter = iter->next)
        numdevices++;

    return numdevices;
}

/**
 * This closes and releases an allocated MTP device.
 * @param device a pointer to the MTP device to release.
 */
void LIBMTP_Release_Device_List(LIBMTP_mtpdevice_t *device)
{
    if(device != NULL) {
        if(device->next != NULL)
            LIBMTP_Release_Device_List(device->next);

        LIBMTP_Release_Device(device);
    }
}

/**
 * This closes and releases an allocated MTP device.
 * @param device a pointer to the MTP device to release.
 */
void LIBMTP_Release_Device(LIBMTP_mtpdevice_t *device)
{
    PTPParams *params = (PTPParams *) device->params;
    PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;

    close_device(ptp_usb, params);
    /* Clear error stack */
    LIBMTP_Clear_Errorstack(device);
    /* Free iconv() converters... */
    iconv_close(params->cd_locale_to_ucs2);
    iconv_close(params->cd_ucs2_to_locale);
    free(ptp_usb);
    ptp_free_params(params);
    free(params);
    free_storage_list(device);
    /* Free extension list... */
    if (device->extensions != NULL) {
        LIBMTP_device_extension_t *tmp = device->extensions;

        while (tmp != NULL) {
            LIBMTP_device_extension_t *next = tmp->next;

            if (tmp->name)
                free(tmp->name);
            free(tmp);
            tmp = next;
        }
    }
    free(device);
}

/**
 * This function dumps out a large chunk of textual information
 * provided from the PTP protocol and additionally some extra
 * MTP-specific information where applicable.
 * @param device a pointer to the MTP device to report info from.
 */
void LIBMTP_Dump_Device_Info(LIBMTP_mtpdevice_t *device)
{
    int i;
    PTPParams *params = (PTPParams *) device->params;
    PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;
    LIBMTP_devicestorage_t *storage = device->storage;
    LIBMTP_device_extension_t *tmpext = device->extensions;

    printf("USB low-level info:\n");
    dump_usbinfo(ptp_usb);
    /* Print out some verbose information */
    printf("Device info:\n");
    printf("   Manufacturer: %s\n", params->deviceinfo.Manufacturer);
    printf("   Model: %s\n", params->deviceinfo.Model);
    printf("   Device version: %s\n", params->deviceinfo.DeviceVersion);
    printf("   Serial number: %s\n", params->deviceinfo.SerialNumber);
    printf("   Vendor extension ID: 0x%08x\n", params->deviceinfo.VendorExtensionID);
    printf("   Vendor extension description: %s\n", params->deviceinfo.VendorExtensionDesc);
    printf("   Detected object size: %d bits\n", device->object_bitsize);
    printf("   Extensions:\n");
    while (tmpext != NULL) {
        printf("        %s: %d.%d\n", tmpext->name, tmpext->major, tmpext->minor);
        tmpext = tmpext->next;
    }
    printf("Supported operations:\n");
    for (i = 0; i < params->deviceinfo.OperationsSupported_len; i++) {
        char txt[256];
        ptp_render_opcode(params, params->deviceinfo.OperationsSupported[i], sizeof(txt), txt);
        printf("   %04x: %s\n", params->deviceinfo.OperationsSupported[i], txt);
    }
    printf("Events supported:\n");
    if (params->deviceinfo.EventsSupported_len == 0)
        printf("   None.\n");
    else
        for (i = 0; i < params->deviceinfo.EventsSupported_len; i++)
            printf("   0x%04x\n", params->deviceinfo.EventsSupported[i]);
    printf("Device Properties Supported:\n");
    for (i = 0; i < params->deviceinfo.DevicePropertiesSupported_len; i++) {
        char const *propdesc = ptp_get_property_description(params,
                               params->deviceinfo.DevicePropertiesSupported[i]);

        if (propdesc != NULL)
            printf("   0x%04x: %s\n", params->deviceinfo.DevicePropertiesSupported[i], propdesc);
        else {
            uint16_t prop = params->deviceinfo.DevicePropertiesSupported[i];
            printf("   0x%04x: Unknown property\n", prop);
        }
    }

    if (ptp_operation_issupported(params,PTP_OC_MTP_GetObjectPropsSupported)) {
        printf("Playable File (Object) Types and Object Properties Supported:\n");
        for (i = 0; i < params->deviceinfo.ImageFormats_len; i++) {
            char txt[256];
            uint16_t ret;
            uint16_t *props = NULL;
            uint32_t propcnt = 0;
            int j;

            ptp_render_ofc(params, params->deviceinfo.ImageFormats[i], sizeof(txt), txt);
            printf("   %04x: %s\n", params->deviceinfo.ImageFormats[i], txt);

            ret = ptp_mtp_getobjectpropssupported (params,
                                                   params->deviceinfo.ImageFormats[i], &propcnt, &props);
            if (ret != PTP_RC_OK) {
                add_ptp_error_to_errorstack(device, ret,
                                            "LIBMTP_Dump_Device_Info(): error on query for object properties.");
                free(props);
                continue;
            }

            for (j = 0; j < propcnt; j++) {
                PTPObjectPropDesc opd;
                int k;

                printf("      %04x: %s", props[j],
                       LIBMTP_Get_Property_Description(map_ptp_property_to_libmtp_property(props[j])));
                /* Get a more verbose description */
                ret = ptp_mtp_getobjectpropdesc(params, props[j], params->deviceinfo.ImageFormats[i], &opd);
                if (ret != PTP_RC_OK) {
                    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL,
                                            "LIBMTP_Dump_Device_Info(): could not get property description.");
                    break;
                }

                if (opd.DataType == PTP_DTC_STR) {
                    printf(" STRING data type");
                    switch (opd.FormFlag) {
                    case PTP_OPFF_DateTime:
                        printf(" DATETIME FORM");
                        break;
                    case PTP_OPFF_RegularExpression:
                        printf(" REGULAR EXPRESSION FORM");
                        break;
                    case PTP_OPFF_LongString:
                        printf(" LONG STRING FORM");
                        break;
                    default:
                        break;
                    }
                } else if (opd.DataType & PTP_DTC_ARRAY_MASK)
                    printf(" array of");

                switch (opd.DataType & (~PTP_DTC_ARRAY_MASK)) {

                case PTP_DTC_UNDEF:
                    printf(" UNDEFINED data type");
                    break;

                case PTP_DTC_INT8:
                    printf(" INT8 data type");
                    switch (opd.FormFlag) {
                    case PTP_OPFF_Range:
                        printf(" range: MIN %d, MAX %d, STEP %d",
                               opd.FORM.Range.MinimumValue.i8,
                               opd.FORM.Range.MaximumValue.i8,
                               opd.FORM.Range.StepSize.i8);
                        break;
                    case PTP_OPFF_Enumeration:
                        printf(" enumeration: ");
                        for (k = 0; k < opd.FORM.Enum.NumberOfValues; k++)
                            printf("%d, ", opd.FORM.Enum.SupportedValue[k].i8);
                        break;
                    case PTP_OPFF_ByteArray:
                        printf(" byte array: ");
                        break;
                    default:
                        printf(" ANY 8BIT VALUE form");
                        break;
                    }
                    break;

                case PTP_DTC_UINT8:
                    printf(" UINT8 data type");
                    switch (opd.FormFlag) {
                    case PTP_OPFF_Range:
                        printf(" range: MIN %d, MAX %d, STEP %d",
                               opd.FORM.Range.MinimumValue.u8,
                               opd.FORM.Range.MaximumValue.u8,
                               opd.FORM.Range.StepSize.u8);
                        break;
                    case PTP_OPFF_Enumeration:
                        printf(" enumeration: ");
                        for (k = 0; k < opd.FORM.Enum.NumberOfValues; k++)
                            printf("%d, ", opd.FORM.Enum.SupportedValue[k].u8);
                        break;
                    case PTP_OPFF_ByteArray:
                        printf(" byte array: ");
                        break;
                    default:
                        printf(" ANY 8BIT VALUE form");
                        break;
                    }
                    break;

                case PTP_DTC_INT16:
                    printf(" INT16 data type");
                    switch (opd.FormFlag) {
                    case PTP_OPFF_Range:
                        printf(" range: MIN %d, MAX %d, STEP %d",
                               opd.FORM.Range.MinimumValue.i16,
                               opd.FORM.Range.MaximumValue.i16,
                               opd.FORM.Range.StepSize.i16);
                        break;
                    case PTP_OPFF_Enumeration:
                        printf(" enumeration: ");
                        for (k = 0; k < opd.FORM.Enum.NumberOfValues; k++)
                            printf("%d, ", opd.FORM.Enum.SupportedValue[k].i16);
                        break;
                    default:
                        printf(" ANY 16BIT VALUE form");
                        break;
                    }
                    break;

                case PTP_DTC_UINT16:
                    printf(" UINT16 data type");
                    switch (opd.FormFlag) {
                    case PTP_OPFF_Range:
                        printf(" range: MIN %d, MAX %d, STEP %d",
                               opd.FORM.Range.MinimumValue.u16,
                               opd.FORM.Range.MaximumValue.u16,
                               opd.FORM.Range.StepSize.u16);
                        break;
                    case PTP_OPFF_Enumeration:
                        printf(" enumeration: ");
                        for (k = 0; k < opd.FORM.Enum.NumberOfValues; k++)
                            printf("%d, ", opd.FORM.Enum.SupportedValue[k].u16);
                        break;
                    default:
                        printf(" ANY 16BIT VALUE form");
                        break;
                    }
                    break;

                case PTP_DTC_INT32:
                    printf(" INT32 data type");
                    switch (opd.FormFlag) {
                    case PTP_OPFF_Range:
                        printf(" range: MIN %d, MAX %d, STEP %d",
                               opd.FORM.Range.MinimumValue.i32,
                               opd.FORM.Range.MaximumValue.i32,
                               opd.FORM.Range.StepSize.i32);
                        break;
                    case PTP_OPFF_Enumeration:
                        printf(" enumeration: ");
                        for (k = 0; k < opd.FORM.Enum.NumberOfValues; k++)
                            printf("%d, ", opd.FORM.Enum.SupportedValue[k].i32);
                        break;
                    default:
                        printf(" ANY 32BIT VALUE form");
                        break;
                    }
                    break;

                case PTP_DTC_UINT32:
                    printf(" UINT32 data type");
                    switch (opd.FormFlag) {
                    case PTP_OPFF_Range:
                        printf(" range: MIN %d, MAX %d, STEP %d",
                               opd.FORM.Range.MinimumValue.u32,
                               opd.FORM.Range.MaximumValue.u32,
                               opd.FORM.Range.StepSize.u32);
                        break;
                    case PTP_OPFF_Enumeration:
                        /* Special pretty-print for FOURCC codes */
                        if (params->deviceinfo.ImageFormats[i] == PTP_OPC_VideoFourCCCodec) {
                            printf(" enumeration of u32 casted FOURCC: ");
                            for (k = 0; k < opd.FORM.Enum.NumberOfValues; k++) {
                                if (opd.FORM.Enum.SupportedValue[k].u32 == 0)
                                    printf("ANY, ");
                                else {
                                    char fourcc[6];
                                    fourcc[0] = (opd.FORM.Enum.SupportedValue[k].u32 >> 24) & 0xFFU;
                                    fourcc[1] = (opd.FORM.Enum.SupportedValue[k].u32 >> 16) & 0xFFU;
                                    fourcc[2] = (opd.FORM.Enum.SupportedValue[k].u32 >> 8) & 0xFFU;
                                    fourcc[3] = opd.FORM.Enum.SupportedValue[k].u32 & 0xFFU;
                                    fourcc[4] = '\n';
                                    fourcc[5] = '\0';
                                    printf("\"%s\", ", fourcc);
                                }
                            }
                        } else {
                            printf(" enumeration: ");
                            for (k = 0; k < opd.FORM.Enum.NumberOfValues; k++)
                                printf("%d, ", opd.FORM.Enum.SupportedValue[k].u32);
                        }
                        break;
                    default:
                        printf(" ANY 32BIT VALUE form");
                        break;
                    }
                    break;

                case PTP_DTC_INT64:
                    printf(" INT64 data type");
                    break;

                case PTP_DTC_UINT64:
                    printf(" UINT64 data type");
                    break;

                case PTP_DTC_INT128:
                    printf(" INT128 data type");
                    break;

                case PTP_DTC_UINT128:
                    printf(" UINT128 data type");
                    break;

                default:
                    printf(" UNKNOWN data type");
                    break;

                }
                if (opd.GetSet)
                    printf(" GET/SET\n");
                else
                    printf(" READ ONLY\n");
                ptp_free_objectpropdesc(&opd);
            }
            free(props);
        }
    }

    if (storage != NULL &&
            ptp_operation_issupported(params,PTP_OC_GetStorageInfo)) {
        printf("Storage Devices:\n");
        while(storage != NULL) {
            printf("   StorageID: 0x%08x\n",storage->id);

            printf("      StorageType: 0x%04x ",storage->StorageType);
            switch (storage->StorageType) {
            case PTP_ST_Undefined:
                printf("(undefined)\n");
                break;
            case PTP_ST_FixedROM:
                printf("fixed ROM storage\n");
                break;
            case PTP_ST_RemovableROM:
                printf("removable ROM storage\n");
                break;
            case PTP_ST_FixedRAM:
                printf("fixed RAM storage\n");
                break;
            case PTP_ST_RemovableRAM:
                printf("removable RAM storage\n");
                break;
            default:
                printf("UNKNOWN storage\n");
                break;
            }

            printf("      FilesystemType: 0x%04x ",storage->FilesystemType);
            switch (storage->FilesystemType) {
            case PTP_FST_Undefined:
                printf("(undefined)\n");
                break;
            case PTP_FST_GenericFlat:
                printf("generic flat filesystem\n");
                break;
            case PTP_FST_GenericHierarchical:
                printf("generic hierarchical\n");
                break;
            case PTP_FST_DCF:
                printf("DCF\n");
                break;
            default:
                printf("UNKNOWN filesystem type\n");
                break;
            }

            printf("      AccessCapability: 0x%04x ",storage->AccessCapability);
            switch (storage->AccessCapability) {
            case PTP_AC_ReadWrite:
                printf("read/write\n");
                break;
            case PTP_AC_ReadOnly:
                printf("read only\n");
                break;
            case PTP_AC_ReadOnly_with_Object_Deletion:
                printf("read only + object deletion\n");
                break;
            default:
                printf("UNKNOWN access capability\n");
                break;
            }

            printf("      MaxCapacity: %llu\n", (long long unsigned int) storage->MaxCapacity);
            printf("      FreeSpaceInBytes: %llu\n", (long long unsigned int) storage->FreeSpaceInBytes);
            printf("      FreeSpaceInObjects: %llu\n", (long long unsigned int) storage->FreeSpaceInObjects);
            printf("      StorageDescription: %s\n",storage->StorageDescription);
            printf("      VolumeIdentifier: %s\n",storage->VolumeIdentifier);
            storage = storage->next;
        }
    }

    printf("Special directories:\n");
    printf("   Default music folder: 0x%08x\n", device->default_music_folder);
    printf("   Default playlist folder: 0x%08x\n", device->default_playlist_folder);
    printf("   Default picture folder: 0x%08x\n", device->default_picture_folder);
    printf("   Default video folder: 0x%08x\n", device->default_video_folder);
    printf("   Default organizer folder: 0x%08x\n", device->default_organizer_folder);
    printf("   Default zencast folder: 0x%08x\n", device->default_zencast_folder);
    printf("   Default album folder: 0x%08x\n", device->default_album_folder);
    printf("   Default text folder: 0x%08x\n", device->default_text_folder);
}

/**
 * This resets a device in case it supports the <code>PTP_OC_ResetDevice</code>
 * operation code (0x1010).
 * @param device a pointer to the device to reset.
 * @return 0 on success, any other value means failure.
 */
int LIBMTP_Reset_Device(LIBMTP_mtpdevice_t *device)
{
    PTPParams *params = (PTPParams *) device->params;
    uint16_t ret;

    if (!ptp_operation_issupported(params,PTP_OC_ResetDevice)) {
        add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL,
                                "LIBMTP_Reset_Device(): device does not support resetting.");
        return -1;
    }
    ret = ptp_resetdevice(params);
    if (ret != PTP_RC_OK) {
        add_ptp_error_to_errorstack(device, ret, "Error resetting.");
        return -1;
    }
    return 0;
}

/**
 * This retrieves the manufacturer name of an MTP device.
 * @param device a pointer to the device to get the manufacturer name for.
 * @return a newly allocated UTF-8 string representing the manufacturer name.
 *         The string must be freed by the caller after use. If the call
 *         was unsuccessful this will contain NULL.
 */
char *LIBMTP_Get_Manufacturername(LIBMTP_mtpdevice_t *device)
{
    char *retmanuf = NULL;
    PTPParams *params = (PTPParams *) device->params;

    if (params->deviceinfo.Manufacturer != NULL)
        retmanuf = strdup(params->deviceinfo.Manufacturer);
    return retmanuf;
}

/**
 * This retrieves the model name (often equal to product name)
 * of an MTP device.
 * @param device a pointer to the device to get the model name for.
 * @return a newly allocated UTF-8 string representing the model name.
 *         The string must be freed by the caller after use. If the call
 *         was unsuccessful this will contain NULL.
 */
char *LIBMTP_Get_Modelname(LIBMTP_mtpdevice_t *device)
{
    char *retmodel = NULL;
    PTPParams *params = (PTPParams *) device->params;

    if (params->deviceinfo.Model != NULL)
        retmodel = strdup(params->deviceinfo.Model);
    return retmodel;
}

/**
 * This retrieves the serial number of an MTP device.
 * @param device a pointer to the device to get the serial number for.
 * @return a newly allocated UTF-8 string representing the serial number.
 *         The string must be freed by the caller after use. If the call
 *         was unsuccessful this will contain NULL.
 */
char *LIBMTP_Get_Serialnumber(LIBMTP_mtpdevice_t *device)
{
    char *retnumber = NULL;
    PTPParams *params = (PTPParams *) device->params;

    if (params->deviceinfo.SerialNumber != NULL)
        retnumber = strdup(params->deviceinfo.SerialNumber);
    return retnumber;
}

/**
 * This retrieves the device version (hardware and firmware version) of an
 * MTP device.
 * @param device a pointer to the device to get the device version for.
 * @return a newly allocated UTF-8 string representing the device version.
 *         The string must be freed by the caller after use. If the call
 *         was unsuccessful this will contain NULL.
 */
char *LIBMTP_Get_Deviceversion(LIBMTP_mtpdevice_t *device)
{
    char *retversion = NULL;
    PTPParams *params = (PTPParams *) device->params;

    if (params->deviceinfo.DeviceVersion != NULL)
        retversion = strdup(params->deviceinfo.DeviceVersion);
    return retversion;
}

/**
 * This retrieves the "friendly name" of an MTP device. Usually
 * this is simply the name of the owner or something like
 * "John Doe's Digital Audio Player". This property should be supported
 * by all MTP devices.
 * @param device a pointer to the device to get the friendly name for.
 * @return a newly allocated UTF-8 string representing the friendly name.
 *         The string must be freed by the caller after use.
 * @see LIBMTP_Set_Friendlyname()
 */
char *LIBMTP_Get_Friendlyname(LIBMTP_mtpdevice_t *device)
{
    PTPPropertyValue propval;
    char *retstring = NULL;
    PTPParams *params = (PTPParams *) device->params;
    uint16_t ret;

    if (!ptp_property_issupported(params, PTP_DPC_MTP_DeviceFriendlyName))
        return NULL;

    ret = ptp_getdevicepropvalue(params, PTP_DPC_MTP_DeviceFriendlyName, &propval, PTP_DTC_STR);
    if (ret != PTP_RC_OK) {
        add_ptp_error_to_errorstack(device, ret, "Error getting friendlyname.");
        return NULL;
    }
    if (propval.str != NULL) {
        retstring = strdup(propval.str);
        free(propval.str);
    }
    return retstring;
}

/**
 * Sets the "friendly name" of an MTP device.
 * @param device a pointer to the device to set the friendly name for.
 * @param friendlyname the new friendly name for the device.
 * @return 0 on success, any other value means failure.
 * @see LIBMTP_Get_Friendlyname()
 */
int LIBMTP_Set_Friendlyname(LIBMTP_mtpdevice_t *device,
                            char const * const friendlyname)
{
    PTPPropertyValue propval;
    PTPParams *params = (PTPParams *) device->params;
    uint16_t ret;

    if (!ptp_property_issupported(params, PTP_DPC_MTP_DeviceFriendlyName))
        return -1;
    propval.str = (char *) friendlyname;
    ret = ptp_setdevicepropvalue(params, PTP_DPC_MTP_DeviceFriendlyName, &propval, PTP_DTC_STR);
    if (ret != PTP_RC_OK) {
        add_ptp_error_to_errorstack(device, ret, "Error setting friendlyname.");
        return -1;
    }
    return 0;
}

/**
 * This retrieves the syncronization partner of an MTP device. This
 * property should be supported by all MTP devices.
 * @param device a pointer to the device to get the sync partner for.
 * @return a newly allocated UTF-8 string representing the synchronization
 *         partner. The string must be freed by the caller after use.
 * @see LIBMTP_Set_Syncpartner()
 */
char *LIBMTP_Get_Syncpartner(LIBMTP_mtpdevice_t *device)
{
    PTPPropertyValue propval;
    char *retstring = NULL;
    PTPParams *params = (PTPParams *) device->params;
    uint16_t ret;

    if (!ptp_property_issupported(params, PTP_DPC_MTP_SynchronizationPartner))
        return NULL;

    ret = ptp_getdevicepropvalue(params, PTP_DPC_MTP_SynchronizationPartner, &propval, PTP_DTC_STR);
    if (ret != PTP_RC_OK) {
        add_ptp_error_to_errorstack(device, ret, "Error getting syncpartner.");
        return NULL;
    }
    if (propval.str != NULL) {
        retstring = strdup(propval.str);
        free(propval.str);
    }
    return retstring;
}

/**
 * Sets the synchronization partner of an MTP device. Note that
 * we have no idea what the effect of setting this to "foobar"
 * may be. But the general idea seems to be to tell which program
 * shall synchronize with this device and tell others to leave
 * it alone.
 * @param device a pointer to the device to set the sync partner for.
 * @param syncpartner the new synchronization partner for the device.
 * @return 0 on success, any other value means failure.
 * @see LIBMTP_Get_Syncpartner()
 */
int LIBMTP_Set_Syncpartner(LIBMTP_mtpdevice_t *device,
                           char const * const syncpartner)
{
    PTPPropertyValue propval;
    PTPParams *params = (PTPParams *) device->params;
    uint16_t ret;

    if (!ptp_property_issupported(params, PTP_DPC_MTP_SynchronizationPartner))
        return -1;
    propval.str = (char *) syncpartner;
    ret = ptp_setdevicepropvalue(params, PTP_DPC_MTP_SynchronizationPartner, &propval, PTP_DTC_STR);
    if (ret != PTP_RC_OK) {
        add_ptp_error_to_errorstack(device, ret, "Error setting syncpartner.");
        return -1;
    }
    return 0;
}

/**
 * This function retrieves the current battery level on the device.
 * @param device a pointer to the device to get the battery level for.
 * @param maximum_level a pointer to a variable that will hold the
 *        maximum level of the battery if the call was successful.
 * @param current_level a pointer to a variable that will hold the
 *        current level of the battery if the call was successful.
 *        A value of 0 means that the device is on external power.
 * @return 0 if the storage info was successfully retrieved, any other
 *        means failure. A typical cause of failure is that
 *        the device does not support the battery level property.
 */
int LIBMTP_Get_Batterylevel(LIBMTP_mtpdevice_t *device,
                            uint8_t * const maximum_level,
                            uint8_t * const current_level)
{
    PTPPropertyValue propval;
    uint16_t ret;
    PTPParams *params = (PTPParams *) device->params;
    PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;

    *maximum_level = 0;
    *current_level = 0;

    if (FLAG_BROKEN_BATTERY_LEVEL(ptp_usb) ||
            !ptp_property_issupported(params, PTP_DPC_BatteryLevel))
        return -1;

    ret = ptp_getdevicepropvalue(params, PTP_DPC_BatteryLevel, &propval, PTP_DTC_UINT8);
    if (ret != PTP_RC_OK) {
        add_ptp_error_to_errorstack(device, ret,
                                    "LIBMTP_Get_Batterylevel(): could not get device property value.");
        return -1;
    }

    *maximum_level = device->maximum_battery_level;
    *current_level = propval.u8;

    return 0;
}

/**
 * This function returns the secure time as an XML document string from
 * the device.
 * @param device a pointer to the device to get the secure time for.
 * @param sectime the secure time string as an XML document or NULL if the call
 *         failed or the secure time property is not supported. This string
 *         must be <code>free()</code>:ed by the caller after use.
 * @return 0 on success, any other value means failure.
 */
int LIBMTP_Get_Secure_Time(LIBMTP_mtpdevice_t *device, char ** const sectime)
{
    return get_device_unicode_property(device, sectime, PTP_DPC_MTP_SecureTime);
}

/**
 * This function returns the device (public key) certificate as an
 * XML document string from the device.
 * @param device a pointer to the device to get the device certificate for.
 * @param devcert the device certificate as an XML string or NULL if the call
 *        failed or the device certificate property is not supported. This
 *        string must be <code>free()</code>:ed by the caller after use.
 * @return 0 on success, any other value means failure.
 */
int LIBMTP_Get_Device_Certificate(LIBMTP_mtpdevice_t *device, char ** const devcert)
{
    return get_device_unicode_property(device, devcert, PTP_DPC_MTP_DeviceCertificate);
}

/**
 * This function retrieves a list of supported file types, i.e. the file
 * types that this device claims it supports, e.g. audio file types that
 * the device can play etc. This list is mitigated to
 * inlcude the file types that libmtp can handle, i.e. it will not list
 * filetypes that libmtp will handle internally like playlists and folders.
 * @param device a pointer to the device to get the filetype capabilities for.
 * @param filetypes a pointer to a pointer that will hold the list of
 *        supported filetypes if the call was successful. This list must
 *        be <code>free()</code>:ed by the caller after use.
 * @param length a pointer to a variable that will hold the length of the
 *        list of supported filetypes if the call was successful.
 * @return 0 on success, any other value means failure.
 * @see LIBMTP_Get_Filetype_Description()
 */
int LIBMTP_Get_Supported_Filetypes(LIBMTP_mtpdevice_t *device, uint16_t ** const filetypes,
                                   uint16_t * const length)
{
    PTPParams *params = (PTPParams *) device->params;
    PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;
    uint16_t *localtypes;
    uint16_t localtypelen;
    uint32_t i;

    /* This is more memory than needed if there are unknown types, but what the heck. */
    localtypes = (uint16_t *) malloc(params->deviceinfo.ImageFormats_len * sizeof(uint16_t));
    localtypelen = 0;

    for (i = 0; i < params->deviceinfo.ImageFormats_len; i++) {
        uint16_t localtype = map_ptp_type_to_libmtp_type(params->deviceinfo.ImageFormats[i]);
        if (localtype != LIBMTP_FILETYPE_UNKNOWN) {
            localtypes[localtypelen] = localtype;
            localtypelen++;
        }
    }
    /* The forgotten Ogg support on YP-10 and others... */
    if (FLAG_OGG_IS_UNKNOWN(ptp_usb)) {
        localtypes = (uint16_t *) realloc(localtypes, (params->deviceinfo.ImageFormats_len + 1) * sizeof(uint16_t));
        localtypes[localtypelen] = LIBMTP_FILETYPE_OGG;
        localtypelen++;
    }
    /* The forgotten FLAC support on Cowon iAudio S9 and others... */
    if (FLAG_FLAC_IS_UNKNOWN(ptp_usb)) {
        localtypes = (uint16_t *) realloc(localtypes, (params->deviceinfo.ImageFormats_len + 1) * sizeof(uint16_t));
        localtypes[localtypelen] = LIBMTP_FILETYPE_FLAC;
        localtypelen++;
    }

    *filetypes = localtypes;
    *length = localtypelen;

    return 0;
}

/**
 * This function checks if the device has some specific capabilities, in
 * order to avoid calling APIs that may disturb the device.
 *
 * @param device a pointer to the device to check the capability on.
 * @param cap the capability to check.
 * @return 0 if not supported, any other value means the device has the
 * requested capability.
 */
int LIBMTP_Check_Capability(LIBMTP_mtpdevice_t *device, LIBMTP_devicecap_t cap)
{
    switch (cap) {
    case LIBMTP_DEVICECAP_GetPartialObject:
        return  ptp_operation_issupported(device->params, PTP_OC_GetPartialObject) ||
                ptp_operation_issupported(device->params, PTP_OC_ANDROID_GetPartialObject64);
    case LIBMTP_DEVICECAP_SendPartialObject:
        return ptp_operation_issupported(device->params, PTP_OC_ANDROID_SendPartialObject);
    case LIBMTP_DEVICECAP_EditObjects:
        return  ptp_operation_issupported(device->params, PTP_OC_ANDROID_TruncateObject) &&
                ptp_operation_issupported(device->params, PTP_OC_ANDROID_BeginEditObject) &&
                ptp_operation_issupported(device->params, PTP_OC_ANDROID_EndEditObject);
        /*
         * Handle other capabilities here, this is also a good place to
         * blacklist some advanced operations on specific devices if need
         * be.
         */

    default:
        break;
    }
    return 0;
}

/**
 * This returns the error stack for a device in case you
 * need to either reference the error numbers (e.g. when
 * creating multilingual apps with multiple-language text
 * representations for each error number) or when you need
 * to build a multi-line error text widget or something like
 * that. You need to call the <code>LIBMTP_Clear_Errorstack</code>
 * to clear it when you're finished with it.
 * @param device a pointer to the MTP device to get the error
 *        stack for.
 * @return the error stack or NULL if there are no errors
 *         on the stack.
 * @see LIBMTP_Clear_Errorstack()
 * @see LIBMTP_Dump_Errorstack()
 */
LIBMTP_error_t *LIBMTP_Get_Errorstack(LIBMTP_mtpdevice_t *device)
{
    if (device == NULL) {
        LIBMTP_ERROR("LIBMTP PANIC: Trying to get the error stack of a NULL device!\n");
        return NULL;
    }
    return device->errorstack;
}

/**
 * This function clears the error stack of a device and frees
 * any memory used by it. Call this when you're finished with
 * using the errors.
 * @param device a pointer to the MTP device to clear the error
 *        stack for.
 */
void LIBMTP_Clear_Errorstack(LIBMTP_mtpdevice_t *device)
{
    if (device == NULL) {
        LIBMTP_ERROR("LIBMTP PANIC: Trying to clear the error stack of a NULL device!\n");
        return;
    }

    LIBMTP_error_t *tmp = device->errorstack;

    while (tmp != NULL) {
        LIBMTP_error_t *tmp2;

        free(tmp->error_text);
        tmp2 = tmp;
        tmp = tmp->next;
        free(tmp2);
    }
    device->errorstack = NULL;
}

/**
 * This function dumps the error stack to <code>stderr</code>.
 * (You still have to clear the stack though.)
 * @param device a pointer to the MTP device to dump the error
 *        stack for.
 */
void LIBMTP_Dump_Errorstack(LIBMTP_mtpdevice_t *device)
{
    if (device == NULL) {
        LIBMTP_ERROR("LIBMTP PANIC: Trying to dump the error stack of a NULL device!\n");
        return;
    }

    LIBMTP_error_t *tmp = device->errorstack;

    while (tmp != NULL) {
        if (tmp->error_text != NULL)
            LIBMTP_ERROR("Error %d: %s\n", tmp->errornumber, tmp->error_text);
        else
            LIBMTP_ERROR("Error %d: (unknown)\n", tmp->errornumber);
        tmp = tmp->next;
    }
}
