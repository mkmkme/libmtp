
/**
 * To read events sent by the device, repeatedly call this function from a secondary
 * thread until the return value is < 0.
 *
 * @param device a pointer to the MTP device to poll for events.
 * @param event contains a pointer to be filled in with the event retrieved if the call
 * is successful.
 * @param out1 contains the param1 value from the raw event.
 * @return 0 on success, any other value means the polling loop shall be
 * terminated immediately for this session.
 */
int LIBMTP_Read_Event(LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, uint32_t *out1)
{
    /*
     * FIXME: Potential race-condition here, if client deallocs device
     * while we're *not* waiting for input. As we'll be waiting for
     * input most of the time, it's unlikely but still worth considering
     * for improvement. Also we cannot affect the state of the cache etc
     * unless we know we are the sole user on the device. A spinlock or
     * mutex in the LIBMTP_mtpdevice_t is needed for this to work.
     */
    PTPParams *params = (PTPParams *) device->params;
    PTPContainer ptp_event;
    uint16_t ret = ptp_usb_event_wait(params, &ptp_event);
    uint16_t code;
    uint32_t session_id;
    uint32_t param1;

    /* Device is closing down or other fatal stuff, exit thread */
    if (ret != PTP_RC_OK)
        return -1;

    *event = LIBMTP_EVENT_NONE;

    /* Process the event */
    code = ptp_event.Code;
    session_id = ptp_event.SessionID;
    param1 = ptp_event.Param1;

    switch(code) {
    case PTP_EC_Undefined:
        LIBMTP_INFO("Received event PTP_EC_Undefined in session %u\n", session_id);
        break;
    case PTP_EC_CancelTransaction:
        LIBMTP_INFO("Received event PTP_EC_CancelTransaction in session %u\n", session_id);
        break;
    case PTP_EC_ObjectAdded:
        LIBMTP_INFO("Received event PTP_EC_ObjectAdded in session %u\n", session_id);
        *event = LIBMTP_EVENT_OBJECT_ADDED;
        *out1 = param1;
        break;
    case PTP_EC_ObjectRemoved:
        LIBMTP_INFO("Received event PTP_EC_ObjectRemoved in session %u\n", session_id);
        *event = LIBMTP_EVENT_OBJECT_REMOVED;
        *out1 = param1;
        break;
    case PTP_EC_StoreAdded:
        LIBMTP_INFO("Received event PTP_EC_StoreAdded in session %u\n", session_id);
        /* TODO: rescan storages */
        *event = LIBMTP_EVENT_STORE_ADDED;
        *out1 = param1;
        break;
    case PTP_EC_StoreRemoved:
        LIBMTP_INFO("Received event PTP_EC_StoreRemoved in session %u\n", session_id);
        /* TODO: rescan storages */
        *event = LIBMTP_EVENT_STORE_REMOVED;
        *out1 = param1;
        break;
    case PTP_EC_DevicePropChanged:
        LIBMTP_INFO("Received event PTP_EC_DevicePropChanged in session %u\n", session_id);
        /* TODO: update device properties */
        break;
    case PTP_EC_ObjectInfoChanged:
        LIBMTP_INFO("Received event PTP_EC_ObjectInfoChanged in session %u\n", session_id);
        /* TODO: rescan object cache or just for this one object */
        break;
    case PTP_EC_DeviceInfoChanged:
        LIBMTP_INFO("Received event PTP_EC_DeviceInfoChanged in session %u\n", session_id);
        /* TODO: update device info */
        break;
    case PTP_EC_RequestObjectTransfer:
        LIBMTP_INFO("Received event PTP_EC_RequestObjectTransfer in session %u\n", session_id);
        break;
    case PTP_EC_StoreFull:
        LIBMTP_INFO("Received event PTP_EC_StoreFull in session %u\n", session_id);
        break;
    case PTP_EC_DeviceReset:
        LIBMTP_INFO("Received event PTP_EC_DeviceReset in session %u\n", session_id);
        break;
    case PTP_EC_StorageInfoChanged :
        LIBMTP_INFO( "Received event PTP_EC_StorageInfoChanged in session %u\n", session_id);
        /* TODO: update storage info */
        break;
    case PTP_EC_CaptureComplete :
        LIBMTP_INFO( "Received event PTP_EC_CaptureComplete in session %u\n", session_id);
        break;
    case PTP_EC_UnreportedStatus :
        LIBMTP_INFO( "Received event PTP_EC_UnreportedStatus in session %u\n", session_id);
        break;
    default :
        LIBMTP_INFO( "Received unknown event in session %u\n", session_id);
        break;
    }

    return 0;
}
