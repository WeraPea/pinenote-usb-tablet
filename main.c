// SPDX-License-Identifier: GPL-3.0-or-later
#include "libevdev-1.0/libevdev/libevdev.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/hidraw.h>
#include <linux/input.h>
#include <linux/usb/ch9.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <usbg/function/hid.h>
#include <usbg/usbg.h>

#define USBG_VENDOR 0x1d6b
#define USBG_PRODUCT 0x0104
#define W9013_VENDOR 0x2d1f
#define W9013_PRODUCT 0x0095

#define WS8100_PEN_NAME "ws8100_pen"

static char report_desc[] = {
    // hid-decode /dev/hidraw0
    // w9013 2D1F:0095
    0x05, 0x0d,       // Usage Page (Digitizers)
    0x09, 0x02,       // Usage (Pen)
    0xa1, 0x01,       // Collection (Application)
    0x85, 0x01,       //  Report ID (1) // added in for bluetooth pen buttons
    0x09, 0x20,       //  Usage (Stylus)
    0xa1, 0x00,       //  Collection (Physical)
    0x09, 0x44,       //   Usage (Barrel Switch)
    0x09, 0x5a,       //   Usage (Secondary Barrel Switch)
    0x09, 0x45,       //   Usage (Eraser)
    0x09, 0x00,       //   Usage (Undefined)
    0x09, 0x00,       //   Usage (Undefined)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x01,       //   Logical Maximum (1)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x05,       //   Report Count (5)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x95, 0x03,       //   Report Count (3)
    0x81, 0x03,       //   Input (Cnst,Var,Abs)
    0xc0,             //  End Collection
    0x85, 0x02,       //  Report ID (2)  // seems to be the only one actually
    0x09, 0x20,       //  Usage (Stylus) // reported by the digitizer
    0xa1, 0x00,       //  Collection (Physical)
    0x09, 0x42,       //   Usage (Tip Switch)
    0x09, 0x44,       //   Usage (Barrel Switch)
    0x09, 0x45,       //   Usage (Eraser)
    0x09, 0x3c,       //   Usage (Invert)
    0x09, 0x5a,       //   Usage (Secondary Barrel Switch)
    0x09, 0x32,       //   Usage (In Range)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x01,       //   Logical Maximum (1)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x06,       //   Report Count (6)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x95, 0x02,       //   Report Count (2)
    0x81, 0x03,       //   Input (Cnst,Var,Abs)
    0x05, 0x01,       //   Usage Page (Generic Desktop)
    0x09, 0x30,       //   Usage (X)
    0x26, 0xe6, 0x51, //   Logical Maximum (20966)
    0x46, 0xe6, 0x51, //   Physical Maximum (20966)
    0x65, 0x11,       //   Unit (SILinear: cm)
    0x55, 0x0d,       //   Unit Exponent (-3)
    0x75, 0x10,       //   Report Size (16)
    0x95, 0x01,       //   Report Count (1)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x09, 0x31,       //   Usage (Y)
    0x26, 0x6d, 0x3d, //   Logical Maximum (15725)
    0x46, 0x6d, 0x3d, //   Physical Maximum (15725)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x45, 0x00,       //   Physical Maximum (0)
    0x65, 0x00,       //   Unit (None)
    0x55, 0x00,       //   Unit Exponent (0)
    0x05, 0x0d,       //   Usage Page (Digitizers)
    0x09, 0x30,       //   Usage (Tip Pressure)
    0x26, 0xff, 0x0f, //   Logical Maximum (4095)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x06, 0x00, 0xff, //   Usage Page (Vendor Defined Page 1)
    0x09, 0x04,       //   Usage (Vendor Usage 0x04)
    0x75, 0x08,       //   Report Size (8)
    0x26, 0xff, 0x00, //   Logical Maximum (255)
    0x46, 0xff, 0x00, //   Physical Maximum (255)
    0x65, 0x11,       //   Unit (SILinear: cm)
    0x55, 0x0e,       //   Unit Exponent (-2)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x05, 0x0d,       //   Usage Page (Digitizers)
    0x09, 0x3d,       //   Usage (X Tilt)
    0x75, 0x10,       //   Report Size (16)
    0x16, 0xd8, 0xdc, //   Logical Minimum (-9000)
    0x26, 0x28, 0x23, //   Logical Maximum (9000)
    0x36, 0xd8, 0xdc, //   Physical Minimum (-9000)
    0x46, 0x28, 0x23, //   Physical Maximum (9000)
    0x65, 0x14,       //   Unit (EnglishRotation: deg)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x09, 0x3e,       //   Usage (Y Tilt)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x65, 0x00,       //   Unit (None)
    0x55, 0x00,       //   Unit Exponent (0)
    0x15, 0x00,       //   Logical Minimum (0)
    0x35, 0x00,       //   Physical Minimum (0)
    0x45, 0x00,       //   Physical Maximum (0)
    0x05, 0x01,       //   Usage Page (Generic Desktop)
    0x09, 0x32,       //   Usage (Z)
    0x75, 0x10,       //   Report Size (16)
    0x16, 0x01, 0xff, //   Logical Minimum (-255)
    0x25, 0x00,       //   Logical Maximum (0)
    0x36, 0x01, 0xff, //   Physical Minimum (-255)
    0x45, 0x00,       //   Physical Maximum (0)
    0x65, 0x11,       //   Unit (SILinear: cm)
    0x55, 0x0e,       //   Unit Exponent (-2)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x15, 0x00,       //   Logical Minimum (0)
    0x35, 0x00,       //   Physical Minimum (0)
    0x65, 0x00,       //   Unit (None)
    0x55, 0x00,       //   Unit Exponent (0)
    0xc0,             //  End Collection
    0x09, 0x00,       //  Usage (Undefined)
    0x75, 0x08,       //  Report Size (8)
    0x26, 0xff, 0x00, //  Logical Maximum (255)
    0xb1, 0x12,       //  Feature (Data,Var,Abs,NonLin)
    0x85, 0x03,       //  Report ID (3)
    0x09, 0x00,       //  Usage (Undefined)
    0x95, 0x12,       //  Report Count (18)
    0xb1, 0x12,       //  Feature (Data,Var,Abs,NonLin)
    0x85, 0x04,       //  Report ID (4)
    0x09, 0x00,       //  Usage (Undefined)
    0xb1, 0x02,       //  Feature (Data,Var,Abs)
    0x85, 0x05,       //  Report ID (5)
    0x09, 0x00,       //  Usage (Undefined)
    0x95, 0x04,       //  Report Count (4)
    0xb1, 0x02,       //  Feature (Data,Var,Abs)
    0x85, 0x06,       //  Report ID (6)
    0x09, 0x00,       //  Usage (Undefined)
    0x95, 0x24,       //  Report Count (36)
    0xb1, 0x02,       //  Feature (Data,Var,Abs)
    0x85, 0x16,       //  Report ID (22)
    0x09, 0x00,       //  Usage (Undefined)
    0x15, 0x00,       //  Logical Minimum (0)
    0x26, 0xff, 0x00, //  Logical Maximum (255)
    0x95, 0x06,       //  Report Count (6)
    0xb1, 0x02,       //  Feature (Data,Var,Abs)
    0x85, 0x17,       //  Report ID (23)
    0x09, 0x00,       //  Usage (Undefined)
    0x95, 0x0c,       //  Report Count (12)
    0xb1, 0x02,       //  Feature (Data,Var,Abs)
    0x85, 0x19,       //  Report ID (25)
    0x09, 0x00,       //  Usage (Undefined)
    0x95, 0x01,       //  Report Count (1)
    0xb1, 0x02,       //  Feature (Data,Var,Abs)
    0xc0,             // End Collection
    0x06, 0x00, 0xff, // Usage Page (Vendor Defined Page 1)
    0x09, 0x00,       // Usage (Undefined)
    0xa1, 0x01,       // Collection (Application)
    0x85, 0x09,       //  Report ID (9)
    0x05, 0x0d,       //  Usage Page (Digitizers)
    0x09, 0x20,       //  Usage (Stylus)
    0xa1, 0x00,       //  Collection (Physical)
    0x09, 0x42,       //   Usage (Tip Switch)
    0x09, 0x44,       //   Usage (Barrel Switch)
    0x09, 0x45,       //   Usage (Eraser)
    0x09, 0x3c,       //   Usage (Invert)
    0x09, 0x00,       //   Usage (Undefined)
    0x09, 0x32,       //   Usage (In Range)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x01,       //   Logical Maximum (1)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x06,       //   Report Count (6)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x95, 0x02,       //   Report Count (2)
    0x81, 0x03,       //   Input (Cnst,Var,Abs)
    0x05, 0x01,       //   Usage Page (Generic Desktop)
    0x09, 0x30,       //   Usage (X)
    0x26, 0xe6, 0x51, //   Logical Maximum (20966)
    0x46, 0xe6, 0x51, //   Physical Maximum (20966)
    0x65, 0x11,       //   Unit (SILinear: cm)
    0x55, 0x0d,       //   Unit Exponent (-3)
    0x75, 0x10,       //   Report Size (16)
    0x95, 0x01,       //   Report Count (1)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x09, 0x31,       //   Usage (Y)
    0x26, 0x6d, 0x3d, //   Logical Maximum (15725)
    0x46, 0x6d, 0x3d, //   Physical Maximum (15725)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x45, 0x00,       //   Physical Maximum (0)
    0x65, 0x00,       //   Unit (None)
    0x55, 0x00,       //   Unit Exponent (0)
    0x05, 0x0d,       //   Usage Page (Digitizers)
    0x09, 0x30,       //   Usage (Tip Pressure)
    0x26, 0xff, 0x0f, //   Logical Maximum (4095)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x06, 0x00, 0xff, //   Usage Page (Vendor Defined Page 1)
    0x09, 0x04,       //   Usage (Vendor Usage 0x04)
    0x75, 0x08,       //   Report Size (8)
    0x26, 0xff, 0x00, //   Logical Maximum (255)
    0x46, 0xff, 0x00, //   Physical Maximum (255)
    0x65, 0x11,       //   Unit (SILinear: cm)
    0x55, 0x0e,       //   Unit Exponent (-2)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x05, 0x0d,       //   Usage Page (Digitizers)
    0x09, 0x3d,       //   Usage (X Tilt)
    0x75, 0x10,       //   Report Size (16)
    0x16, 0xd8, 0xdc, //   Logical Minimum (-9000)
    0x26, 0x28, 0x23, //   Logical Maximum (9000)
    0x36, 0xd8, 0xdc, //   Physical Minimum (-9000)
    0x46, 0x28, 0x23, //   Physical Maximum (9000)
    0x65, 0x14,       //   Unit (EnglishRotation: deg)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x09, 0x3e,       //   Usage (Y Tilt)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x65, 0x00,       //   Unit (None)
    0x55, 0x00,       //   Unit Exponent (0)
    0x15, 0x00,       //   Logical Minimum (0)
    0x35, 0x00,       //   Physical Minimum (0)
    0x45, 0x00,       //   Physical Maximum (0)
    0xc0,             //  End Collection
    0x09, 0x00,       //  Usage (Undefined)
    0x75, 0x08,       //  Report Size (8)
    0x95, 0x03,       //  Report Count (3)
    0x26, 0xff, 0x00, //  Logical Maximum (255)
    0xb1, 0x12,       //  Feature (Data,Var,Abs,NonLin)
    0xc0,             // End Collection
    0x06, 0x00, 0xff, // Usage Page (Vendor Defined Page 1)
    0x09, 0x02,       // Usage (Vendor Usage 2)
    0xa1, 0x01,       // Collection (Application)
    0x85, 0x07,       //  Report ID (7)
    0x09, 0x00,       //  Usage (Undefined)
    0x96, 0x09, 0x01, //  Report Count (265)
    0xb1, 0x02,       //  Feature (Data,Var,Abs)
    0x85, 0x08,       //  Report ID (8)
    0x09, 0x00,       //  Usage (Undefined)
    0x95, 0x03,       //  Report Count (3)
    0x81, 0x02,       //  Input (Data,Var,Abs)
    0x09, 0x00,       //  Usage (Undefined)
    0xb1, 0x02,       //  Feature (Data,Var,Abs)
    0x85, 0x0e,       //  Report ID (14)
    0x09, 0x00,       //  Usage (Undefined)
    0x96, 0x0a, 0x01, //  Report Count (266)
    0xb1, 0x02,       //  Feature (Data,Var,Abs)
    0xc0,             // End Collection
    0x05, 0x0d,       // Usage Page (Digitizers)
    0x09, 0x02,       // Usage (Pen)
    0xa1, 0x01,       // Collection (Application)
    0x85, 0x1a,       //  Report ID (26)
    0x09, 0x20,       //  Usage (Stylus)
    0xa1, 0x00,       //  Collection (Physical)
    0x09, 0x42,       //   Usage (Tip Switch)
    0x09, 0x44,       //   Usage (Barrel Switch)
    0x09, 0x45,       //   Usage (Eraser)
    0x09, 0x3c,       //   Usage (Invert)
    0x09, 0x5a,       //   Usage (Secondary Barrel Switch)
    0x09, 0x32,       //   Usage (In Range)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x01,       //   Logical Maximum (1)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x06,       //   Report Count (6)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x09, 0x38,       //   Usage (Transducer Index)
    0x25, 0x03,       //   Logical Maximum (3)
    0x75, 0x02,       //   Report Size (2)
    0x95, 0x01,       //   Report Count (1)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x05, 0x01,       //   Usage Page (Generic Desktop)
    0x09, 0x30,       //   Usage (X)
    0x26, 0xe6, 0x51, //   Logical Maximum (20966)
    0x46, 0xe6, 0x51, //   Physical Maximum (20966)
    0x65, 0x11,       //   Unit (SILinear: cm)
    0x55, 0x0d,       //   Unit Exponent (-3)
    0x75, 0x10,       //   Report Size (16)
    0x95, 0x01,       //   Report Count (1)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x09, 0x31,       //   Usage (Y)
    0x26, 0x6d, 0x3d, //   Logical Maximum (15725)
    0x46, 0x6d, 0x3d, //   Physical Maximum (15725)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x05, 0x0d,       //   Usage Page (Digitizers)
    0x09, 0x30,       //   Usage (Tip Pressure)
    0x26, 0xff, 0x0f, //   Logical Maximum (4095)
    0x46, 0xb0, 0x0f, //   Physical Maximum (4016)
    0x66, 0x11, 0xe1, //   Unit (SILinear: cm * g * s⁻²)
    0x55, 0x02,       //   Unit Exponent (2)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x06, 0x00, 0xff, //   Usage Page (Vendor Defined Page 1)
    0x09, 0x04,       //   Usage (Vendor Usage 0x04)
    0x75, 0x08,       //   Report Size (8)
    0x26, 0xff, 0x00, //   Logical Maximum (255)
    0x46, 0xff, 0x00, //   Physical Maximum (255)
    0x65, 0x11,       //   Unit (SILinear: cm)
    0x55, 0x0e,       //   Unit Exponent (-2)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x05, 0x0d,       //   Usage Page (Digitizers)
    0x09, 0x3d,       //   Usage (X Tilt)
    0x75, 0x10,       //   Report Size (16)
    0x16, 0xd8, 0xdc, //   Logical Minimum (-9000)
    0x26, 0x28, 0x23, //   Logical Maximum (9000)
    0x36, 0xd8, 0xdc, //   Physical Minimum (-9000)
    0x46, 0x28, 0x23, //   Physical Maximum (9000)
    0x65, 0x14,       //   Unit (EnglishRotation: deg)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x09, 0x3e,       //   Usage (Y Tilt)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x65, 0x00,       //   Unit (None)
    0x55, 0x00,       //   Unit Exponent (0)
    0x15, 0x00,       //   Logical Minimum (0)
    0x35, 0x00,       //   Physical Minimum (0)
    0x45, 0x00,       //   Physical Maximum (0)
    0x05, 0x01,       //   Usage Page (Generic Desktop)
    0x09, 0x32,       //   Usage (Z)
    0x75, 0x10,       //   Report Size (16)
    0x95, 0x01,       //   Report Count (1)
    0x16, 0x01, 0xff, //   Logical Minimum (-255)
    0x25, 0x00,       //   Logical Maximum (0)
    0x36, 0x01, 0xff, //   Physical Minimum (-255)
    0x45, 0x00,       //   Physical Maximum (0)
    0x65, 0x11,       //   Unit (SILinear: cm)
    0x55, 0x0e,       //   Unit Exponent (-2)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x15, 0x00,       //   Logical Minimum (0)
    0x35, 0x00,       //   Physical Minimum (0)
    0x65, 0x00,       //   Unit (None)
    0x55, 0x00,       //   Unit Exponent (0)
    0xc0,             //  End Collection
    0xc0,             // End Collection
    0x06, 0x00, 0xff, // Usage Page (Vendor Defined Page 1)
    0x09, 0x00,       // Usage (Undefined)
    0xa1, 0x01,       // Collection (Application)
    0x85, 0x1b,       //  Report ID (27)
    0x05, 0x0d,       //  Usage Page (Digitizers)
    0x09, 0x20,       //  Usage (Stylus)
    0xa1, 0x00,       //  Collection (Physical)
    0x09, 0x42,       //   Usage (Tip Switch)
    0x09, 0x44,       //   Usage (Barrel Switch)
    0x09, 0x45,       //   Usage (Eraser)
    0x09, 0x3c,       //   Usage (Invert)
    0x09, 0x00,       //   Usage (Undefined)
    0x09, 0x32,       //   Usage (In Range)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x01,       //   Logical Maximum (1)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x06,       //   Report Count (6)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x09, 0x38,       //   Usage (Transducer Index)
    0x25, 0x03,       //   Logical Maximum (3)
    0x75, 0x02,       //   Report Size (2)
    0x95, 0x01,       //   Report Count (1)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x05, 0x01,       //   Usage Page (Generic Desktop)
    0x09, 0x30,       //   Usage (X)
    0x26, 0xe6, 0x51, //   Logical Maximum (20966)
    0x46, 0xe6, 0x51, //   Physical Maximum (20966)
    0x65, 0x11,       //   Unit (SILinear: cm)
    0x55, 0x0d,       //   Unit Exponent (-3)
    0x75, 0x10,       //   Report Size (16)
    0x95, 0x01,       //   Report Count (1)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x09, 0x31,       //   Usage (Y)
    0x26, 0x6d, 0x3d, //   Logical Maximum (15725)
    0x46, 0x6d, 0x3d, //   Physical Maximum (15725)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x45, 0x00,       //   Physical Maximum (0)
    0x65, 0x00,       //   Unit (None)
    0x55, 0x00,       //   Unit Exponent (0)
    0x05, 0x0d,       //   Usage Page (Digitizers)
    0x09, 0x30,       //   Usage (Tip Pressure)
    0x26, 0xff, 0x0f, //   Logical Maximum (4095)
    0x46, 0xb0, 0x0f, //   Physical Maximum (4016)
    0x66, 0x11, 0xe1, //   Unit (SILinear: cm * g * s⁻²)
    0x55, 0x02,       //   Unit Exponent (2)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x06, 0x00, 0xff, //   Usage Page (Vendor Defined Page 1)
    0x09, 0x04,       //   Usage (Vendor Usage 0x04)
    0x75, 0x08,       //   Report Size (8)
    0x26, 0xff, 0x00, //   Logical Maximum (255)
    0x46, 0xff, 0x00, //   Physical Maximum (255)
    0x65, 0x11,       //   Unit (SILinear: cm)
    0x55, 0x0e,       //   Unit Exponent (-2)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x05, 0x0d,       //   Usage Page (Digitizers)
    0x09, 0x3d,       //   Usage (X Tilt)
    0x75, 0x10,       //   Report Size (16)
    0x16, 0xd8, 0xdc, //   Logical Minimum (-9000)
    0x26, 0x28, 0x23, //   Logical Maximum (9000)
    0x36, 0xd8, 0xdc, //   Physical Minimum (-9000)
    0x46, 0x28, 0x23, //   Physical Maximum (9000)
    0x65, 0x14,       //   Unit (EnglishRotation: deg)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x09, 0x3e,       //   Usage (Y Tilt)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x65, 0x00,       //   Unit (None)
    0x55, 0x00,       //   Unit Exponent (0)
    0x15, 0x00,       //   Logical Minimum (0)
    0x35, 0x00,       //   Physical Minimum (0)
    0x45, 0x00,       //   Physical Maximum (0)
    0x05, 0x01,       //   Usage Page (Generic Desktop)
    0x09, 0x32,       //   Usage (Z)
    0x75, 0x10,       //   Report Size (16)
    0x95, 0x01,       //   Report Count (1)
    0x16, 0x01, 0xff, //   Logical Minimum (-255)
    0x25, 0x00,       //   Logical Maximum (0)
    0x36, 0x01, 0xff, //   Physical Minimum (-255)
    0x45, 0x00,       //   Physical Maximum (0)
    0x65, 0x11,       //   Unit (SILinear: cm)
    0x55, 0x0e,       //   Unit Exponent (-2)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x15, 0x00,       //   Logical Minimum (0)
    0x35, 0x00,       //   Physical Minimum (0)
    0x65, 0x00,       //   Unit (None)
    0x55, 0x00,       //   Unit Exponent (0)
    0xc0,             //  End Collection
    0xc0,             // End Collection
};

typedef struct {
  usbg_state *s;
  usbg_gadget *g;
  usbg_config *c;
  usbg_function *f_hid;
} usbg_context;

int initUSB(usbg_context *usb_ctx) {
  int usbg_ret = -EINVAL;

  struct usbg_gadget_attrs g_attrs = {
      .bcdUSB = 0x0200,
      .bDeviceClass = USB_CLASS_PER_INTERFACE,
      .bDeviceSubClass = 0x00,
      .bDeviceProtocol = 0x00,
      .bMaxPacketSize0 = 64, /* Max allowed ep0 packet size */
      .idVendor = USBG_VENDOR,
      .idProduct = USBG_PRODUCT,
      .bcdDevice = 0x0100, /* Verson of device */
  };
  struct usbg_gadget_strs g_strs = {
      .serial = "fedcba9876543210", /* Serial number */
      .manufacturer = "Pine64",     /* Manufacturer */
      .product = "PineNote"         /* Product string */
  };
  struct usbg_config_strs c_strs = {.configuration = "1xHID"};
  struct usbg_f_hid_attrs f_attrs = {
      .protocol = 2,
      .report_desc =
          {
              .desc = report_desc,
              .len = sizeof(report_desc),
          },
      .report_length = 15,
      .subclass = 1,
  };

  usbg_ret = usbg_init("/sys/kernel/config", &usb_ctx->s);
  if (usbg_ret != USBG_SUCCESS) {
    fprintf(stderr, "Error on usbg init\n");
    fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
            usbg_strerror(usbg_ret));
    goto out1;
  }
  usbg_ret =
      usbg_create_gadget(usb_ctx->s, "g1", &g_attrs, &g_strs, &usb_ctx->g);
  if (usbg_ret != USBG_SUCCESS) {
    fprintf(stderr, "Error creating gadget\n");
    fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
            usbg_strerror(usbg_ret));
    goto out2;
  }
  usbg_ret = usbg_create_function(usb_ctx->g, USBG_F_HID, "usb0", &f_attrs,
                                  &usb_ctx->f_hid);
  if (usbg_ret != USBG_SUCCESS) {
    fprintf(stderr, "Error creating function\n");
    fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
            usbg_strerror(usbg_ret));
    goto out2;
  }
  usbg_ret = usbg_create_config(usb_ctx->g, 1, "The only one", NULL, &c_strs,
                                &usb_ctx->c);
  if (usbg_ret != USBG_SUCCESS) {
    fprintf(stderr, "Error creating config\n");
    fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
            usbg_strerror(usbg_ret));
    goto out2;
  }
  usbg_ret = usbg_add_config_function(usb_ctx->c, "some_name", usb_ctx->f_hid);
  if (usbg_ret != USBG_SUCCESS) {
    fprintf(stderr, "Error adding function\n");
    fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
            usbg_strerror(usbg_ret));
    goto out2;
  }
  usbg_ret = usbg_enable_gadget(usb_ctx->g, DEFAULT_UDC);
  if (usbg_ret != USBG_SUCCESS) {
    fprintf(stderr, "Error enabling gadget\n");
    fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
            usbg_strerror(usbg_ret));
    goto out2;
  }
  usbg_ret = 0;
  goto out1;

out2:
  usbg_cleanup(usb_ctx->s);
  usb_ctx->s = NULL;

out1:
  return usbg_ret;
}

int cleanupUSB(usbg_context *usb_ctx) {
  if (usb_ctx->g) {
    usbg_disable_gadget(usb_ctx->g);
    usbg_rm_gadget(usb_ctx->g, USBG_RM_RECURSE);
  }
  if (usb_ctx->s) {
    usbg_cleanup(usb_ctx->s);
  }
  return 0;
}

int find_hidraw_device(char *device, int16_t vid, int16_t pid) {
  int fd;
  struct hidraw_devinfo hidinfo;
  char path[20];

  for (int x = 0; x < 16; x++) {
    sprintf(path, "/dev/hidraw%d", x);

    if ((fd = open(path, O_RDWR | O_NONBLOCK)) == -1) {
      continue;
    }

    ioctl(fd, HIDIOCGRAWINFO, &hidinfo);
    if (hidinfo.vendor == vid && hidinfo.product == pid) {
      printf("Found %s at: %s\n", device, path);
      return fd;
    }

    close(fd);
  }

  return -1;
}

int find_evdev_device(char *name, struct libevdev **out_dev) {
  int fd;
  char path[20];
  struct libevdev *dev = NULL;

  for (int x = 0; x < 16; x++) {
    sprintf(path, "/dev/input/event%d", x);

    if ((fd = open(path, O_RDWR | O_NONBLOCK)) == -1) {
      continue;
    }
    if (libevdev_new_from_fd(fd, &dev) < 0) {
      libevdev_free(dev);
      continue;
    };
    if (strcmp(libevdev_get_name(dev), name) == 0) {
      printf("Found %s at: %s\n", name, path);
      *out_dev = dev;
      return 0;
    }

    libevdev_free(dev);
  }

  return -1;
}

int handle_ws8100_pen_events(struct input_event ev, unsigned char *buttons,
                             int out_fd) {
  if (ev.type == EV_KEY) {
    int bit = -1;
    bool invert = false; // used for when double press event is passed between
                         // tool down and up to reproduce it with one output
                         // instead of two.
                         // (see drivers/input/misc/ws8100-pen.c
                         // of the kernel)
    switch (ev.code) {
    case BTN_TOOL_RUBBER: // button 1 press and release, except it is kept
                          // pressed during double press
      bit = 0;
      break;
    case KEY_MACRO1: // button 1 double press
      bit = 0;
      invert = true;
      break;
    case BTN_TOOL_PEN: // button 2 press and release, except it is kept pressed
                       // during double press
      bit = 1;
      break;
    case KEY_MACRO2: // button 2 double press
      bit = 1;
      invert = true;
      break;
    case BTN_STYLUS3: // button 3 short press
      bit = 2;
      break;
    case KEY_SLEEP: // button 3 long press
      bit = 3;
      break;
    case KEY_MACRO3: // button 3 double press
      bit = 4;
      break;
    }

    if (bit >= 0) {
      if (ev.value ^ invert) {
        buttons[1] |= 1 << bit;
      } else {
        buttons[1] &= ~(1 << bit);
      }
      if (write(out_fd, buttons, 2) != 2) {
        perror("Write failed");
        return -1;
      }
    }
  }
  return 0;
}

bool volatile keepRunning = true;

void intHandler(int dummy) { keepRunning = false; }

int main() {
  signal(SIGINT, intHandler);

  int w9013, out_fd, evdev_rc, ws8100_pen_fd;
  unsigned char w9013_buffer[15];
  ssize_t bytes = 0;
  unsigned char buttons[2] = {1, 0};
  usbg_context usb_ctx = {0};

  struct libevdev *ws8100_pen = NULL;
  if (initUSB(&usb_ctx) < 0) {
    fprintf(stderr, "Failed to init usb gadget");
    goto out4;
  }

  w9013 = find_hidraw_device("w9013 digitizer", W9013_VENDOR, W9013_PRODUCT);
  if (w9013 < 0) {
    fprintf(stderr, "Failed to find w9013 digitizer");
    goto out4;
  }

  out_fd = open("/dev/hidg0", O_WRONLY);
  if (out_fd < 0) {
    perror("Failed to open /dev/hidg0");
    goto out3;
  }

  evdev_rc = find_evdev_device(WS8100_PEN_NAME, &ws8100_pen);
  if (evdev_rc < 0) {
    fprintf(stderr, "Failed to find ws8100_pen");
    goto out2;
  }

  ws8100_pen_fd = libevdev_get_fd(ws8100_pen);
  evdev_rc = libevdev_grab(ws8100_pen, LIBEVDEV_GRAB);
  if (evdev_rc < 0) {
    fprintf(stderr, "Failed to grab ws8100_pen");
    goto out1;
  }

  struct pollfd fds[2] = {
      {.fd = ws8100_pen_fd, .events = POLLIN},
      {.fd = w9013, .events = POLLIN},
  };

  while (keepRunning) {
    int r = poll(fds, 2, -1);
    if (r < 0) {
      if (errno == EINTR)
        continue;
      perror("Failed to poll for events");
      break;
    }
    if (fds[0].revents & POLLIN) {
      struct input_event ev;
      do {
        evdev_rc =
            libevdev_next_event(ws8100_pen, LIBEVDEV_READ_FLAG_NORMAL, &ev);
        if (evdev_rc == LIBEVDEV_READ_STATUS_SYNC) {
          printf("::::::::::::::::::::: dropped ::::::::::::::::::::::\n");
          while (evdev_rc == LIBEVDEV_READ_STATUS_SYNC) {
            evdev_rc =
                libevdev_next_event(ws8100_pen, LIBEVDEV_READ_FLAG_SYNC, &ev);
            handle_ws8100_pen_events(ev, buttons, out_fd);
          }
          printf("::::::::::::::::::::: re-synced ::::::::::::::::::::::\n");
        } else if (evdev_rc == LIBEVDEV_READ_STATUS_SUCCESS) {
          handle_ws8100_pen_events(ev, buttons, out_fd);
        } else {
          fprintf(stderr, "Failed to handle events: %s\n", strerror(-evdev_rc));
          goto out1;
        }
      } while (libevdev_has_event_pending(ws8100_pen));
    }
    if (fds[1].revents & POLLIN) {
      while ((bytes = read(w9013, w9013_buffer, sizeof(w9013_buffer))) > 0) {
        if (write(out_fd, w9013_buffer, bytes) != bytes) {
          perror("Write failed");
          goto out1;
        }
      }
      if (bytes < 0 && errno != EAGAIN) {
        perror("Read failed");
        goto out1;
      }
    }
  }
out1:
  libevdev_grab(ws8100_pen, LIBEVDEV_UNGRAB);
  libevdev_free(ws8100_pen);
  close(ws8100_pen_fd);
out2:
  close(out_fd);
out3:
  close(w9013);
out4:
  cleanupUSB(&usb_ctx);
  return evdev_rc;
}
