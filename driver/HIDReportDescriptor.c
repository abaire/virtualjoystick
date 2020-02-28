#include "HIDReportDescriptor.h"

#include "common.h"

HID_REPORT_DESCRIPTOR ReportDescriptor[] = {
    0x05, USAGE_PAGE_GENERIC_DESKTOP, // USAGE_PAGE (Generic Desktop)
    0x15, 0x00, // LOGICAL_MINIMUM (0)
    0x09, 0x04, // USAGE (Joystick)
    0xa1, 0x01, // COLLECTION (Application)
    0x85, REPORTID_JOYSTICK, //   REPORT_ID (Joystick)

    0x05, USAGE_PAGE_GENERIC_DESKTOP, //   USAGE_PAGE (Generic)
    0x09, 0x01, //   USAGE (Pointer)
    0xa1, 0x00, //   COLLECTION (Physical)
    0x09, 0x30, //     USAGE (X)
    0x09, 0x31, //     USAGE (Y)
    0x16, 0x01, 0x80, //     LOGICAL_MINIMUM (-32767)
    0x26, 0xFF, 0x7F, //     LOGICAL_MAXIMUM (32767)
    0x75, 0x10, //     REPORT_SIZE (16)
    0x95, 0x02, //     REPORT_COUNT (2)
    0x81, 0x02, //     INPUT (Data,Var,Abs)
    0xc0, //   END_COLLECTION (Pointer)

    0x05, USAGE_PAGE_SIMULATION_CONTROLS, //   USAGE_PAGE (Simulation Controls)
    0x09, USAGE_THROTTLE, //   USAGE (Throttle)
    0x16, 0x01, 0x80, //     LOGICAL_MINIMUM (-32767)
    0x26, 0xFF, 0x7F, //     LOGICAL_MAXIMUM (32767)
    0x75, 0x10, //     REPORT_SIZE (16)
    0x95, 0x01, //   REPORT_COUNT (1)
    0x81, 0x02, //   INPUT (Data,Var,Abs)

    // 0x05, USAGE_PAGE_SIMULATION_CONTROLS, //   USAGE_PAGE (Simulation Controls)
    // 0x09, USAGE_RUDDER, //   USAGE
    // 0x16, 0x01, 0x80, //     LOGICAL_MINIMUM (-32767)
    // 0x26, 0xFF, 0x7F, //     LOGICAL_MAXIMUM (32767)
    // 0x75, 0x10, //     REPORT_SIZE (16)
    // 0x95, 0x01, //   REPORT_COUNT (1)
    // 0x81, 0x02, //   INPUT (Data,Var,Abs)

    0x05, USAGE_PAGE_GENERIC_DESKTOP, //   USAGE_PAGE (generic)
    0x09, USAGE_RX, //   USAGE (Rx)
    0x16, 0x01, 0x80, //     LOGICAL_MINIMUM (-32767)
    0x26, 0xFF, 0x7F, //     LOGICAL_MAXIMUM (32767)
    0x75, 0x10, //     REPORT_SIZE (16)
    0x95, 0x01, //   REPORT_COUNT (1)
    0x81, 0x02, //   INPUT (Data,Var,Abs)

    0x05, USAGE_PAGE_GENERIC_DESKTOP, //   USAGE_PAGE (generic)
    0x09, USAGE_RY, //   USAGE (Ry)
    0x16, 0x01, 0x80, //     LOGICAL_MINIMUM (-32767)
    0x26, 0xFF, 0x7F, //     LOGICAL_MAXIMUM (32767)
    0x75, 0x10, //     REPORT_SIZE (16)
    0x95, 0x01, //   REPORT_COUNT (1)
    0x81, 0x02, //   INPUT (Data,Var,Abs)

    0x05, USAGE_PAGE_GENERIC_DESKTOP, //   USAGE_PAGE (generic)
    0x09, USAGE_RZ, //   USAGE (Rz)
    0x16, 0x01, 0x80, //     LOGICAL_MINIMUM (-32767)
    0x26, 0xFF, 0x7F, //     LOGICAL_MAXIMUM (32767)
    0x75, 0x10, //     REPORT_SIZE (16)
    0x95, 0x01, //   REPORT_COUNT (1)
    0x81, 0x02, //   INPUT (Data,Var,Abs)

    // DirectX expects exactly 1 slider and 1 dial, providing more than 1
    // will cause them to fail to be mapped to the game controller prefs
    // window.

    0x05, USAGE_PAGE_GENERIC_DESKTOP, //   USAGE_PAGE (generic)
    0x09, USAGE_SLIDER, //   USAGE (Slider)
    0x16, 0x01, 0x80, //     LOGICAL_MINIMUM (-32767)
    0x26, 0xFF, 0x7F, //     LOGICAL_MAXIMUM (32767)
    0x75, 0x10, //     REPORT_SIZE (16)
    0x95, 1, //   REPORT_COUNT
    0x81, 0x02, //   INPUT (Data,Var,Abs)

    0x05, USAGE_PAGE_GENERIC_DESKTOP, //   USAGE_PAGE (generic)
    0x09, USAGE_DIAL, //   USAGE (Dial)
    0x16, 0x01, 0x80, //     LOGICAL_MINIMUM (-32767)
    0x26, 0xFF, 0x7F, //     LOGICAL_MAXIMUM (32767)
    0x75, 0x10, //     REPORT_SIZE (16)
    0x95, 1, //   REPORT_COUNT
    0x81, 0x02, //   INPUT (Data,Var,Abs)

    0x05, USAGE_PAGE_GENERIC_DESKTOP, //   USAGE_PAGE (generic)
  0x09, 0x39,        //   USAGE (Hat switch)
  0x15, 0,           //   LOGICAL_MINIMUM (0)
  0x25, 7,      //   LOGICAL_MAXIMUM
  0x75, 0x08,        //   REPORT_SIZE (8)
  0x95, 0x04,        //   REPORT_COUNT (4)
  0x81, 0x02,        //   INPUT (Data,Var,Abs)
    // 0x09, USAGE_HAT_SWITCH, //   USAGE (Hat switch)
    // 0x15, 0xFF, //   LOGICAL_MINIMUM (-1)
    // 0x25, 7, //   LOGICAL_MAXIMUM (7)
    // 0x75, 0x08, //   REPORT_SIZE (8)
    // 0x95, 4, //   REPORT_COUNT
    // 0x81, 0x02, //   INPUT (Data,Var,Abs)

    0x05, USAGE_PAGE_BUTTONS, //   USAGE_PAGE (Button)
    0x19, 0x01, //   USAGE_MINIMUM
    0x29, 32, //   USAGE_MAXIMUM
    0x15, 0x00, //   LOGICAL_MINIMUM (0)
    0x25, 0x01, //   LOGICAL_MAXIMUM (1)
    0x75, 0x01, //   REPORT_SIZE (1)
    0x95, 128, //   REPORT_COUNT (128)
    0x55, 0x00, //   UNIT_EXPONENT (0)
    0x65, 0x00, //   UNIT (None)
    0x81, 0x02, //   INPUT (Data,Var,Abs)

    0xc0, // END_COLLECTION (REPORTID_JOYSTICK)


    //------------------ Vendor Defined ------------------------//
    // Must match structure of _DEVICE_REPORT in common.h.
    // Must also map to the structure described via the USAGE reports above.
    0x06, 0x00, 0xFF, // USAGE_PAGE (Vendor Defined Page 1)
    0x09, 0x01, // USAGE (Vendor Usage 1)
    0xa1, 0x01, // COLLECTION (Application)
    0x85, REPORTID_VENDOR, //   REPORT_ID (Vendor 1)

    0x16, 0x01, 0x80, //   LOGICAL_MINIMUM (-32767)
    0x26, 0xFF, 0x7F, //   LOGICAL_MAXIMUM (32767)
    0x75, 0x10, //   REPORT_SIZE (16)
    0x95, 8, //   REPORT_COUNT: X, Y, Throttle, rX, rY, rZ, Slider, Dial
    0x09, 0x02, //   USAGE (Vendor Usage 1)
    0x91, 0x02, //   OUTPUT (Data,Var,Abs)

    0x15, 0xFF, //   LOGICAL_MINIMUM (-1)
    0x25, 7, //   LOGICAL_MAXIMUM (7)
    0x75, 0x08, //   REPORT_SIZE (8)
    0x95, 4, //   REPORT_COUNT: POV
    0x09, 0x02, //   USAGE (Vendor Usage 1)
    0x91, 0x02, //   OUTPUT (Data,Var,Abs)

    0x15, 0x00, //   LOGICAL_MINIMUM (0)
    0x25, 0x01, //   LOGICAL_MAXIMUM (1)
    0x75, 0x01, //   REPORT_SIZE (1)
    0x95, 128, //   REPORT_COUNT: Buttons (128 bits)
    0x09, 0x02, //   USAGE (Vendor Usage 1)
    0x91, 0x02, //   OUTPUT (Data,Var,Abs)

    0xc0 // END_COLLECTION
};

HID_DESCRIPTOR HidDescriptor = {
    0x09, // length of HID descriptor
    0x21, // descriptor type == HID  0x21
    0x0100, // hid spec release
    0x00, // country code == Not Specified
    0x01, // number of HID class descriptors
    {
        0x22, // report descriptor type 0x22
        sizeof(ReportDescriptor) // total length of report descriptor
    }
};
