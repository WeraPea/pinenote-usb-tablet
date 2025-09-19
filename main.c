// SPDX-License-Identifier: GPL-3.0-or-later
#include "libevdev-1.0/libevdev/libevdev.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/usb/ch9.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <usbg/function/hid.h>
#include <usbg/usbg.h>

#define VENDOR 0x1d6b
#define PRODUCT 0x0104

static char report_desc[] = {
    // hid-decode /dev/hidraw0
    // w9013 2D1F:0095
    0x05, 0x0d,       // Usage Page (Digitizers)
    0x09, 0x02,       // Usage (Pen)
    0xa1, 0x01,       // Collection (Application)
    0x85, 0x01,       //  Report ID (1) // added in for bluetooth pen buttons
    0x09, 0x20,       //  Usage (Stylus)
    0xa1, 0x00,       //  Collection (Physical)
    0x09, 0x42,       //  Usage (Tip Switch)
    0x09, 0x44,       //  Usage (Barrel Switch)
    0x09, 0x45,       //  Usage (Eraser)
    0x15, 0x00,       //  Logical Minimum (0)
    0x25, 0x01,       //  Logical Maximum (1)
    0x75, 0x01,       //  Report Size (1)
    0x95, 0x03,       //  Report Count (3)
    0x81, 0x02,       //  Input (Data,Var,Abs)
    0x95, 0x05,       //  Report Count (5)
    0x81, 0x03,       //  Input (Cnst,Var,Abs)
    0xc0,             // End Collection
    0x85, 0x02,       //  Report ID (2) // seems to be the only one actually reported by the digitizer
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

static int print_event(struct input_event *ev) {
  if (ev->type == EV_SYN)
    printf("Event: time %ld.%06ld, ++++++++++++++++++++ %s +++++++++++++++\n",
           ev->input_event_sec, ev->input_event_usec,
           libevdev_event_type_get_name(ev->type));
  else
    printf("Event: time %ld.%06ld, type %d (%s), code %d (%s), value %d\n",
           ev->input_event_sec, ev->input_event_usec, ev->type,
           libevdev_event_type_get_name(ev->type), ev->code,
           libevdev_event_code_get_name(ev->type, ev->code), ev->value);
  return 0;
}

int main() {
  usbg_state *s;
  usbg_gadget *g;
  usbg_config *c;
  usbg_function *f_hid;
  int ret = -EINVAL;
  int usbg_ret;

  struct usbg_gadget_attrs g_attrs = {
      .bcdUSB = 0x0200,
      .bDeviceClass = USB_CLASS_PER_INTERFACE,
      .bDeviceSubClass = 0x00,
      .bDeviceProtocol = 0x00,
      .bMaxPacketSize0 = 64, /* Max allowed ep0 packet size */
      .idVendor = VENDOR,
      .idProduct = PRODUCT,
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

  usbg_ret = usbg_init("/sys/kernel/config", &s);
  if (usbg_ret != USBG_SUCCESS) {
    fprintf(stderr, "Error on usbg init\n");
    fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
            usbg_strerror(usbg_ret));
    goto out1;
  }

  usbg_ret = usbg_create_gadget(s, "g1", &g_attrs, &g_strs, &g);
  if (usbg_ret != USBG_SUCCESS) {
    fprintf(stderr, "Error creating gadget\n");
    fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
            usbg_strerror(usbg_ret));
    goto out2;
  }
  usbg_ret = usbg_create_function(g, USBG_F_HID, "usb0", &f_attrs, &f_hid);
  if (usbg_ret != USBG_SUCCESS) {
    fprintf(stderr, "Error creating function\n");
    fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
            usbg_strerror(usbg_ret));
    goto out2;
  }

  usbg_ret = usbg_create_config(g, 1, "The only one", NULL, &c_strs, &c);
  if (usbg_ret != USBG_SUCCESS) {
    fprintf(stderr, "Error creating config\n");
    fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
            usbg_strerror(usbg_ret));
    goto out2;
  }

  usbg_ret = usbg_add_config_function(c, "some_name", f_hid);
  if (usbg_ret != USBG_SUCCESS) {
    fprintf(stderr, "Error adding function\n");
    fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
            usbg_strerror(usbg_ret));
    goto out2;
  }

  usbg_ret = usbg_enable_gadget(g, DEFAULT_UDC);
  if (usbg_ret != USBG_SUCCESS) {
    fprintf(stderr, "Error enabling gadget\n");
    fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
            usbg_strerror(usbg_ret));
    goto out2;
  }

  int in_fd, out_fd;
  unsigned char buffer[15];
  ssize_t bytes;

  in_fd = open("/dev/hidraw0", O_RDONLY | O_NONBLOCK);
  if (in_fd < 0) {
    perror("Failed to open /dev/hidraw0");
    goto out2;
  }

  out_fd = open("/dev/hidg0", O_WRONLY);
  if (out_fd < 0) {
    perror("Failed to open /dev/hidg0");
    close(in_fd);
    goto out2;
  }

  unsigned char buttons[2] = {1, 0};

  struct libevdev *dev = NULL;
  const char *file;
  int fd;
  int rc = 1;

  file = "/dev/input/event6";
  fd = open(file, O_RDONLY | O_NONBLOCK);
  if (fd < 0) {
    perror("Failed to open device");
    goto out;
  }

  rc = libevdev_new_from_fd(fd, &dev);
  if (rc < 0) {
    fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-rc));
    goto out;
  }

  printf("Input device ID: bus %#x vendor %#x product %#x\n",
         libevdev_get_id_bustype(dev), libevdev_get_id_vendor(dev),
         libevdev_get_id_product(dev));
  printf("Evdev version: %x\n", libevdev_get_driver_version(dev));
  printf("Input device name: \"%s\"\n", libevdev_get_name(dev));
  printf("Phys location: %s\n", libevdev_get_phys(dev));
  printf("Uniq identifier: %s\n", libevdev_get_uniq(dev));

  do {
    if (libevdev_has_event_pending(dev)) {
      struct input_event ev;
      rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
      if (rc == LIBEVDEV_READ_STATUS_SYNC) {
        printf("::::::::::::::::::::: dropped ::::::::::::::::::::::\n");
        while (rc == LIBEVDEV_READ_STATUS_SYNC) {
          rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_SYNC, &ev);
        }
        printf("::::::::::::::::::::: re-synced ::::::::::::::::::::::\n");
      } else if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
        // print_event(&ev);
        if (ev.type == EV_KEY) {
          int bit = -1;
          switch (ev.code) {
          case BTN_TOOL_RUBBER:
            bit = 0;
            break;
          case BTN_TOOL_PEN:
            bit = 1;
            break;
          case BTN_STYLUS3:
            bit = 2;
            break;
          }
          if (bit >= 0) {
            if (ev.value) {
              buttons[1] |= 1 << bit;
            } else {
              buttons[1] &= ~(1 << bit);
            }
            printf("%08b\n", buttons[1]);
            if (write(out_fd, &buttons, 2) != 2) {
              perror("Write failed");
              goto out;
            }
          }
        }
      }
    }
    if ((bytes = read(in_fd, buffer, sizeof(buffer))) > 0) {
      if (write(out_fd, buffer, bytes) != bytes) {
        perror("Write failed");
        goto out;
      }
    }

  } while (rc == LIBEVDEV_READ_STATUS_SYNC ||
           rc == LIBEVDEV_READ_STATUS_SUCCESS || rc == -EAGAIN);

  if (rc != LIBEVDEV_READ_STATUS_SUCCESS && rc != -EAGAIN) {
    fprintf(stderr, "Failed to handle events: %s\n", strerror(-rc));
    goto out;
  }
out:

  if (bytes < 0) {
    perror("Read failed");
    goto out2;
  }

  libevdev_free(dev);
  close(in_fd);
  close(out_fd);

  ret = 0;
out2:
  usbg_cleanup(s);

out1:
  return ret;
}
