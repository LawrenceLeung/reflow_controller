#define CORE_TEENSY
#if defined(USB_SERIAL)
#include "../usb_serial/core_id.h"
#elif defined(USB_HID)
#include "../usb_hid/core_id.h"
#elif defined(USB_DISK) || defined(USB_DISK_SDFLASH)
#include "../usb_disk/core_id.h"
#endif

