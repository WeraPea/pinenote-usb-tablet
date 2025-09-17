// SPDX-License-Identifier: GPL-3.0-or-later
#include <errno.h>
#include <fcntl.h>
#include <linux/usb/ch9.h>
#include <stdio.h>
#include <unistd.h>
#include <usbg/function/hid.h>
#include <usbg/usbg.h>

#define VENDOR 0x1d6b
#define PRODUCT 0x0104

static char report_desc[] = {
    // hid-decode /dev/hidraw0
    // w9013 2D1F:0095
    0x05, 0x0d,       // Usage Page (Digitizers)             0
    0x09, 0x02,       // Usage (Pen)                         2
    0xa1, 0x01,       // Collection (Application)            4
    0x85, 0x02,       //  Report ID (2)                      6
    0x09, 0x20,       //  Usage (Stylus)                     8
    0xa1, 0x00,       //  Collection (Physical)              10
    0x09, 0x42,       //   Usage (Tip Switch)                12
    0x09, 0x44,       //   Usage (Barrel Switch)             14
    0x09, 0x45,       //   Usage (Eraser)                    16
    0x09, 0x3c,       //   Usage (Invert)                    18
    0x09, 0x5a,       //   Usage (Secondary Barrel Switch)   20
    0x09, 0x32,       //   Usage (In Range)                  22
    0x15, 0x00,       //   Logical Minimum (0)               24
    0x25, 0x01,       //   Logical Maximum (1)               26
    0x75, 0x01,       //   Report Size (1)                   28
    0x95, 0x06,       //   Report Count (6)                  30
    0x81, 0x02,       //   Input (Data,Var,Abs)              32
    0x95, 0x02,       //   Report Count (2)                  34
    0x81, 0x03,       //   Input (Cnst,Var,Abs)              36
    0x05, 0x01,       //   Usage Page (Generic Desktop)      38
    0x09, 0x30,       //   Usage (X)                         40
    0x26, 0xe6, 0x51, //   Logical Maximum (20966)           42
    0x46, 0xe6, 0x51, //   Physical Maximum (20966)          45
    0x65, 0x11,       //   Unit (SILinear: cm)               48
    0x55, 0x0d,       //   Unit Exponent (-3)                50
    0x75, 0x10,       //   Report Size (16)                  52
    0x95, 0x01,       //   Report Count (1)                  54
    0x81, 0x02,       //   Input (Data,Var,Abs)              56
    0x09, 0x31,       //   Usage (Y)                         58
    0x26, 0x6d, 0x3d, //   Logical Maximum (15725)           60
    0x46, 0x6d, 0x3d, //   Physical Maximum (15725)          63
    0x81, 0x02,       //   Input (Data,Var,Abs)              66
    0x45, 0x00,       //   Physical Maximum (0)              68
    0x65, 0x00,       //   Unit (None)                       70
    0x55, 0x00,       //   Unit Exponent (0)                 72
    0x05, 0x0d,       //   Usage Page (Digitizers)           74
    0x09, 0x30,       //   Usage (Tip Pressure)              76
    0x26, 0xff, 0x0f, //   Logical Maximum (4095)            78
    0x81, 0x02,       //   Input (Data,Var,Abs)              81
    0x06, 0x00, 0xff, //   Usage Page (Vendor Defined Page 1) 83
    0x09, 0x04,       //   Usage (Vendor Usage 0x04)         86
    0x75, 0x08,       //   Report Size (8)                   88
    0x26, 0xff, 0x00, //   Logical Maximum (255)             90
    0x46, 0xff, 0x00, //   Physical Maximum (255)            93
    0x65, 0x11,       //   Unit (SILinear: cm)               96
    0x55, 0x0e,       //   Unit Exponent (-2)                98
    0x81, 0x02,       //   Input (Data,Var,Abs)              100
    0x05, 0x0d,       //   Usage Page (Digitizers)           102
    0x09, 0x3d,       //   Usage (X Tilt)                    104
    0x75, 0x10,       //   Report Size (16)                  106
    0x16, 0xd8, 0xdc, //   Logical Minimum (-9000)           108
    0x26, 0x28, 0x23, //   Logical Maximum (9000)            111
    0x36, 0xd8, 0xdc, //   Physical Minimum (-9000)          114
    0x46, 0x28, 0x23, //   Physical Maximum (9000)           117
    0x65, 0x14,       //   Unit (EnglishRotation: deg)       120
    0x81, 0x02,       //   Input (Data,Var,Abs)              122
    0x09, 0x3e,       //   Usage (Y Tilt)                    124
    0x81, 0x02,       //   Input (Data,Var,Abs)              126
    0x65, 0x00,       //   Unit (None)                       128
    0x55, 0x00,       //   Unit Exponent (0)                 130
    0x15, 0x00,       //   Logical Minimum (0)               132
    0x35, 0x00,       //   Physical Minimum (0)              134
    0x45, 0x00,       //   Physical Maximum (0)              136
    0x05, 0x01,       //   Usage Page (Generic Desktop)      138
    0x09, 0x32,       //   Usage (Z)                         140
    0x75, 0x10,       //   Report Size (16)                  142
    0x16, 0x01, 0xff, //   Logical Minimum (-255)            144
    0x25, 0x00,       //   Logical Maximum (0)               147
    0x36, 0x01, 0xff, //   Physical Minimum (-255)           149
    0x45, 0x00,       //   Physical Maximum (0)              152
    0x65, 0x11,       //   Unit (SILinear: cm)               154
    0x55, 0x0e,       //   Unit Exponent (-2)                156
    0x81, 0x02,       //   Input (Data,Var,Abs)              158
    0x15, 0x00,       //   Logical Minimum (0)               160
    0x35, 0x00,       //   Physical Minimum (0)              162
    0x65, 0x00,       //   Unit (None)                       164
    0x55, 0x00,       //   Unit Exponent (0)                 166
    0xc0,             //  End Collection                     168
    0x09, 0x00,       //  Usage (Undefined)                  169
    0x75, 0x08,       //  Report Size (8)                    171
    0x26, 0xff, 0x00, //  Logical Maximum (255)              173
    0xb1, 0x12,       //  Feature (Data,Var,Abs,NonLin)      176
    0x85, 0x03,       //  Report ID (3)                      178
    0x09, 0x00,       //  Usage (Undefined)                  180
    0x95, 0x12,       //  Report Count (18)                  182
    0xb1, 0x12,       //  Feature (Data,Var,Abs,NonLin)      184
    0x85, 0x04,       //  Report ID (4)                      186
    0x09, 0x00,       //  Usage (Undefined)                  188
    0xb1, 0x02,       //  Feature (Data,Var,Abs)             190
    0x85, 0x05,       //  Report ID (5)                      192
    0x09, 0x00,       //  Usage (Undefined)                  194
    0x95, 0x04,       //  Report Count (4)                   196
    0xb1, 0x02,       //  Feature (Data,Var,Abs)             198
    0x85, 0x06,       //  Report ID (6)                      200
    0x09, 0x00,       //  Usage (Undefined)                  202
    0x95, 0x24,       //  Report Count (36)                  204
    0xb1, 0x02,       //  Feature (Data,Var,Abs)             206
    0x85, 0x16,       //  Report ID (22)                     208
    0x09, 0x00,       //  Usage (Undefined)                  210
    0x15, 0x00,       //  Logical Minimum (0)                212
    0x26, 0xff, 0x00, //  Logical Maximum (255)              214
    0x95, 0x06,       //  Report Count (6)                   217
    0xb1, 0x02,       //  Feature (Data,Var,Abs)             219
    0x85, 0x17,       //  Report ID (23)                     221
    0x09, 0x00,       //  Usage (Undefined)                  223
    0x95, 0x0c,       //  Report Count (12)                  225
    0xb1, 0x02,       //  Feature (Data,Var,Abs)             227
    0x85, 0x19,       //  Report ID (25)                     229
    0x09, 0x00,       //  Usage (Undefined)                  231
    0x95, 0x01,       //  Report Count (1)                   233
    0xb1, 0x02,       //  Feature (Data,Var,Abs)             235
    0xc0,             // End Collection                      237
    0x06, 0x00, 0xff, // Usage Page (Vendor Defined Page 1)  238
    0x09, 0x00,       // Usage (Undefined)                   241
    0xa1, 0x01,       // Collection (Application)            243
    0x85, 0x09,       //  Report ID (9)                      245
    0x05, 0x0d,       //  Usage Page (Digitizers)            247
    0x09, 0x20,       //  Usage (Stylus)                     249
    0xa1, 0x00,       //  Collection (Physical)              251
    0x09, 0x42,       //   Usage (Tip Switch)                253
    0x09, 0x44,       //   Usage (Barrel Switch)             255
    0x09, 0x45,       //   Usage (Eraser)                    257
    0x09, 0x3c,       //   Usage (Invert)                    259
    0x09, 0x00,       //   Usage (Undefined)                 261
    0x09, 0x32,       //   Usage (In Range)                  263
    0x15, 0x00,       //   Logical Minimum (0)               265
    0x25, 0x01,       //   Logical Maximum (1)               267
    0x75, 0x01,       //   Report Size (1)                   269
    0x95, 0x06,       //   Report Count (6)                  271
    0x81, 0x02,       //   Input (Data,Var,Abs)              273
    0x95, 0x02,       //   Report Count (2)                  275
    0x81, 0x03,       //   Input (Cnst,Var,Abs)              277
    0x05, 0x01,       //   Usage Page (Generic Desktop)      279
    0x09, 0x30,       //   Usage (X)                         281
    0x26, 0xe6, 0x51, //   Logical Maximum (20966)           283
    0x46, 0xe6, 0x51, //   Physical Maximum (20966)          286
    0x65, 0x11,       //   Unit (SILinear: cm)               289
    0x55, 0x0d,       //   Unit Exponent (-3)                291
    0x75, 0x10,       //   Report Size (16)                  293
    0x95, 0x01,       //   Report Count (1)                  295
    0x81, 0x02,       //   Input (Data,Var,Abs)              297
    0x09, 0x31,       //   Usage (Y)                         299
    0x26, 0x6d, 0x3d, //   Logical Maximum (15725)           301
    0x46, 0x6d, 0x3d, //   Physical Maximum (15725)          304
    0x81, 0x02,       //   Input (Data,Var,Abs)              307
    0x45, 0x00,       //   Physical Maximum (0)              309
    0x65, 0x00,       //   Unit (None)                       311
    0x55, 0x00,       //   Unit Exponent (0)                 313
    0x05, 0x0d,       //   Usage Page (Digitizers)           315
    0x09, 0x30,       //   Usage (Tip Pressure)              317
    0x26, 0xff, 0x0f, //   Logical Maximum (4095)            319
    0x81, 0x02,       //   Input (Data,Var,Abs)              322
    0x06, 0x00, 0xff, //   Usage Page (Vendor Defined Page 1) 324
    0x09, 0x04,       //   Usage (Vendor Usage 0x04)         327
    0x75, 0x08,       //   Report Size (8)                   329
    0x26, 0xff, 0x00, //   Logical Maximum (255)             331
    0x46, 0xff, 0x00, //   Physical Maximum (255)            334
    0x65, 0x11,       //   Unit (SILinear: cm)               337
    0x55, 0x0e,       //   Unit Exponent (-2)                339
    0x81, 0x02,       //   Input (Data,Var,Abs)              341
    0x05, 0x0d,       //   Usage Page (Digitizers)           343
    0x09, 0x3d,       //   Usage (X Tilt)                    345
    0x75, 0x10,       //   Report Size (16)                  347
    0x16, 0xd8, 0xdc, //   Logical Minimum (-9000)           349
    0x26, 0x28, 0x23, //   Logical Maximum (9000)            352
    0x36, 0xd8, 0xdc, //   Physical Minimum (-9000)          355
    0x46, 0x28, 0x23, //   Physical Maximum (9000)           358
    0x65, 0x14,       //   Unit (EnglishRotation: deg)       361
    0x81, 0x02,       //   Input (Data,Var,Abs)              363
    0x09, 0x3e,       //   Usage (Y Tilt)                    365
    0x81, 0x02,       //   Input (Data,Var,Abs)              367
    0x65, 0x00,       //   Unit (None)                       369
    0x55, 0x00,       //   Unit Exponent (0)                 371
    0x15, 0x00,       //   Logical Minimum (0)               373
    0x35, 0x00,       //   Physical Minimum (0)              375
    0x45, 0x00,       //   Physical Maximum (0)              377
    0xc0,             //  End Collection                     379
    0x09, 0x00,       //  Usage (Undefined)                  380
    0x75, 0x08,       //  Report Size (8)                    382
    0x95, 0x03,       //  Report Count (3)                   384
    0x26, 0xff, 0x00, //  Logical Maximum (255)              386
    0xb1, 0x12,       //  Feature (Data,Var,Abs,NonLin)      389
    0xc0,             // End Collection                      391
    0x06, 0x00, 0xff, // Usage Page (Vendor Defined Page 1)  392
    0x09, 0x02,       // Usage (Vendor Usage 2)              395
    0xa1, 0x01,       // Collection (Application)            397
    0x85, 0x07,       //  Report ID (7)                      399
    0x09, 0x00,       //  Usage (Undefined)                  401
    0x96, 0x09, 0x01, //  Report Count (265)                 403
    0xb1, 0x02,       //  Feature (Data,Var,Abs)             406
    0x85, 0x08,       //  Report ID (8)                      408
    0x09, 0x00,       //  Usage (Undefined)                  410
    0x95, 0x03,       //  Report Count (3)                   412
    0x81, 0x02,       //  Input (Data,Var,Abs)               414
    0x09, 0x00,       //  Usage (Undefined)                  416
    0xb1, 0x02,       //  Feature (Data,Var,Abs)             418
    0x85, 0x0e,       //  Report ID (14)                     420
    0x09, 0x00,       //  Usage (Undefined)                  422
    0x96, 0x0a, 0x01, //  Report Count (266)                 424
    0xb1, 0x02,       //  Feature (Data,Var,Abs)             427
    0xc0,             // End Collection                      429
    0x05, 0x0d,       // Usage Page (Digitizers)             430
    0x09, 0x02,       // Usage (Pen)                         432
    0xa1, 0x01,       // Collection (Application)            434
    0x85, 0x1a,       //  Report ID (26)                     436
    0x09, 0x20,       //  Usage (Stylus)                     438
    0xa1, 0x00,       //  Collection (Physical)              440
    0x09, 0x42,       //   Usage (Tip Switch)                442
    0x09, 0x44,       //   Usage (Barrel Switch)             444
    0x09, 0x45,       //   Usage (Eraser)                    446
    0x09, 0x3c,       //   Usage (Invert)                    448
    0x09, 0x5a,       //   Usage (Secondary Barrel Switch)   450
    0x09, 0x32,       //   Usage (In Range)                  452
    0x15, 0x00,       //   Logical Minimum (0)               454
    0x25, 0x01,       //   Logical Maximum (1)               456
    0x75, 0x01,       //   Report Size (1)                   458
    0x95, 0x06,       //   Report Count (6)                  460
    0x81, 0x02,       //   Input (Data,Var,Abs)              462
    0x09, 0x38,       //   Usage (Transducer Index)          464
    0x25, 0x03,       //   Logical Maximum (3)               466
    0x75, 0x02,       //   Report Size (2)                   468
    0x95, 0x01,       //   Report Count (1)                  470
    0x81, 0x02,       //   Input (Data,Var,Abs)              472
    0x05, 0x01,       //   Usage Page (Generic Desktop)      474
    0x09, 0x30,       //   Usage (X)                         476
    0x26, 0xe6, 0x51, //   Logical Maximum (20966)           478
    0x46, 0xe6, 0x51, //   Physical Maximum (20966)          481
    0x65, 0x11,       //   Unit (SILinear: cm)               484
    0x55, 0x0d,       //   Unit Exponent (-3)                486
    0x75, 0x10,       //   Report Size (16)                  488
    0x95, 0x01,       //   Report Count (1)                  490
    0x81, 0x02,       //   Input (Data,Var,Abs)              492
    0x09, 0x31,       //   Usage (Y)                         494
    0x26, 0x6d, 0x3d, //   Logical Maximum (15725)           496
    0x46, 0x6d, 0x3d, //   Physical Maximum (15725)          499
    0x81, 0x02,       //   Input (Data,Var,Abs)              502
    0x05, 0x0d,       //   Usage Page (Digitizers)           504
    0x09, 0x30,       //   Usage (Tip Pressure)              506
    0x26, 0xff, 0x0f, //   Logical Maximum (4095)            508
    0x46, 0xb0, 0x0f, //   Physical Maximum (4016)           511
    0x66, 0x11, 0xe1, //   Unit (SILinear: cm * g * s⁻²)     514
    0x55, 0x02,       //   Unit Exponent (2)                 517
    0x81, 0x02,       //   Input (Data,Var,Abs)              519
    0x06, 0x00, 0xff, //   Usage Page (Vendor Defined Page 1) 521
    0x09, 0x04,       //   Usage (Vendor Usage 0x04)         524
    0x75, 0x08,       //   Report Size (8)                   526
    0x26, 0xff, 0x00, //   Logical Maximum (255)             528
    0x46, 0xff, 0x00, //   Physical Maximum (255)            531
    0x65, 0x11,       //   Unit (SILinear: cm)               534
    0x55, 0x0e,       //   Unit Exponent (-2)                536
    0x81, 0x02,       //   Input (Data,Var,Abs)              538
    0x05, 0x0d,       //   Usage Page (Digitizers)           540
    0x09, 0x3d,       //   Usage (X Tilt)                    542
    0x75, 0x10,       //   Report Size (16)                  544
    0x16, 0xd8, 0xdc, //   Logical Minimum (-9000)           546
    0x26, 0x28, 0x23, //   Logical Maximum (9000)            549
    0x36, 0xd8, 0xdc, //   Physical Minimum (-9000)          552
    0x46, 0x28, 0x23, //   Physical Maximum (9000)           555
    0x65, 0x14,       //   Unit (EnglishRotation: deg)       558
    0x81, 0x02,       //   Input (Data,Var,Abs)              560
    0x09, 0x3e,       //   Usage (Y Tilt)                    562
    0x81, 0x02,       //   Input (Data,Var,Abs)              564
    0x65, 0x00,       //   Unit (None)                       566
    0x55, 0x00,       //   Unit Exponent (0)                 568
    0x15, 0x00,       //   Logical Minimum (0)               570
    0x35, 0x00,       //   Physical Minimum (0)              572
    0x45, 0x00,       //   Physical Maximum (0)              574
    0x05, 0x01,       //   Usage Page (Generic Desktop)      576
    0x09, 0x32,       //   Usage (Z)                         578
    0x75, 0x10,       //   Report Size (16)                  580
    0x95, 0x01,       //   Report Count (1)                  582
    0x16, 0x01, 0xff, //   Logical Minimum (-255)            584
    0x25, 0x00,       //   Logical Maximum (0)               587
    0x36, 0x01, 0xff, //   Physical Minimum (-255)           589
    0x45, 0x00,       //   Physical Maximum (0)              592
    0x65, 0x11,       //   Unit (SILinear: cm)               594
    0x55, 0x0e,       //   Unit Exponent (-2)                596
    0x81, 0x02,       //   Input (Data,Var,Abs)              598
    0x15, 0x00,       //   Logical Minimum (0)               600
    0x35, 0x00,       //   Physical Minimum (0)              602
    0x65, 0x00,       //   Unit (None)                       604
    0x55, 0x00,       //   Unit Exponent (0)                 606
    0xc0,             //  End Collection                     608
    0xc0,             // End Collection                      609
    0x06, 0x00, 0xff, // Usage Page (Vendor Defined Page 1)  610
    0x09, 0x00,       // Usage (Undefined)                   613
    0xa1, 0x01,       // Collection (Application)            615
    0x85, 0x1b,       //  Report ID (27)                     617
    0x05, 0x0d,       //  Usage Page (Digitizers)            619
    0x09, 0x20,       //  Usage (Stylus)                     621
    0xa1, 0x00,       //  Collection (Physical)              623
    0x09, 0x42,       //   Usage (Tip Switch)                625
    0x09, 0x44,       //   Usage (Barrel Switch)             627
    0x09, 0x45,       //   Usage (Eraser)                    629
    0x09, 0x3c,       //   Usage (Invert)                    631
    0x09, 0x00,       //   Usage (Undefined)                 633
    0x09, 0x32,       //   Usage (In Range)                  635
    0x15, 0x00,       //   Logical Minimum (0)               637
    0x25, 0x01,       //   Logical Maximum (1)               639
    0x75, 0x01,       //   Report Size (1)                   641
    0x95, 0x06,       //   Report Count (6)                  643
    0x81, 0x02,       //   Input (Data,Var,Abs)              645
    0x09, 0x38,       //   Usage (Transducer Index)          647
    0x25, 0x03,       //   Logical Maximum (3)               649
    0x75, 0x02,       //   Report Size (2)                   651
    0x95, 0x01,       //   Report Count (1)                  653
    0x81, 0x02,       //   Input (Data,Var,Abs)              655
    0x05, 0x01,       //   Usage Page (Generic Desktop)      657
    0x09, 0x30,       //   Usage (X)                         659
    0x26, 0xe6, 0x51, //   Logical Maximum (20966)           661
    0x46, 0xe6, 0x51, //   Physical Maximum (20966)          664
    0x65, 0x11,       //   Unit (SILinear: cm)               667
    0x55, 0x0d,       //   Unit Exponent (-3)                669
    0x75, 0x10,       //   Report Size (16)                  671
    0x95, 0x01,       //   Report Count (1)                  673
    0x81, 0x02,       //   Input (Data,Var,Abs)              675
    0x09, 0x31,       //   Usage (Y)                         677
    0x26, 0x6d, 0x3d, //   Logical Maximum (15725)           679
    0x46, 0x6d, 0x3d, //   Physical Maximum (15725)          682
    0x81, 0x02,       //   Input (Data,Var,Abs)              685
    0x45, 0x00,       //   Physical Maximum (0)              687
    0x65, 0x00,       //   Unit (None)                       689
    0x55, 0x00,       //   Unit Exponent (0)                 691
    0x05, 0x0d,       //   Usage Page (Digitizers)           693
    0x09, 0x30,       //   Usage (Tip Pressure)              695
    0x26, 0xff, 0x0f, //   Logical Maximum (4095)            697
    0x46, 0xb0, 0x0f, //   Physical Maximum (4016)           700
    0x66, 0x11, 0xe1, //   Unit (SILinear: cm * g * s⁻²)     703
    0x55, 0x02,       //   Unit Exponent (2)                 706
    0x81, 0x02,       //   Input (Data,Var,Abs)              708
    0x06, 0x00, 0xff, //   Usage Page (Vendor Defined Page 1) 710
    0x09, 0x04,       //   Usage (Vendor Usage 0x04)         713
    0x75, 0x08,       //   Report Size (8)                   715
    0x26, 0xff, 0x00, //   Logical Maximum (255)             717
    0x46, 0xff, 0x00, //   Physical Maximum (255)            720
    0x65, 0x11,       //   Unit (SILinear: cm)               723
    0x55, 0x0e,       //   Unit Exponent (-2)                725
    0x81, 0x02,       //   Input (Data,Var,Abs)              727
    0x05, 0x0d,       //   Usage Page (Digitizers)           729
    0x09, 0x3d,       //   Usage (X Tilt)                    731
    0x75, 0x10,       //   Report Size (16)                  733
    0x16, 0xd8, 0xdc, //   Logical Minimum (-9000)           735
    0x26, 0x28, 0x23, //   Logical Maximum (9000)            738
    0x36, 0xd8, 0xdc, //   Physical Minimum (-9000)          741
    0x46, 0x28, 0x23, //   Physical Maximum (9000)           744
    0x65, 0x14,       //   Unit (EnglishRotation: deg)       747
    0x81, 0x02,       //   Input (Data,Var,Abs)              749
    0x09, 0x3e,       //   Usage (Y Tilt)                    751
    0x81, 0x02,       //   Input (Data,Var,Abs)              753
    0x65, 0x00,       //   Unit (None)                       755
    0x55, 0x00,       //   Unit Exponent (0)                 757
    0x15, 0x00,       //   Logical Minimum (0)               759
    0x35, 0x00,       //   Physical Minimum (0)              761
    0x45, 0x00,       //   Physical Maximum (0)              763
    0x05, 0x01,       //   Usage Page (Generic Desktop)      765
    0x09, 0x32,       //   Usage (Z)                         767
    0x75, 0x10,       //   Report Size (16)                  769
    0x95, 0x01,       //   Report Count (1)                  771
    0x16, 0x01, 0xff, //   Logical Minimum (-255)            773
    0x25, 0x00,       //   Logical Maximum (0)               776
    0x36, 0x01, 0xff, //   Physical Minimum (-255)           778
    0x45, 0x00,       //   Physical Maximum (0)              781
    0x65, 0x11,       //   Unit (SILinear: cm)               783
    0x55, 0x0e,       //   Unit Exponent (-2)                785
    0x81, 0x02,       //   Input (Data,Var,Abs)              787
    0x15, 0x00,       //   Logical Minimum (0)               789
    0x35, 0x00,       //   Physical Minimum (0)              791
    0x65, 0x00,       //   Unit (None)                       793
    0x55, 0x00,       //   Unit Exponent (0)                 795
    0xc0,             //  End Collection                     797
    0xc0,             // End Collection                      798
};

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

  in_fd = open("/dev/hidraw0", O_RDONLY);
  if (in_fd < 0) {
    fprintf(stderr, "Failed to open /dev/hidraw0\n");
    goto out2;
  }

  out_fd = open("/dev/hidg0", O_WRONLY);
  if (out_fd < 0) {
    fprintf(stderr, "Failed to open /dev/hidg0\n");
    close(in_fd);
    goto out2;
  }

  while ((bytes = read(in_fd, buffer, sizeof(buffer))) > 0) {
    if (write(out_fd, buffer, bytes) != bytes) {
      fprintf(stderr, "Write failed\n");
      break;
    }
  }

  if (bytes < 0) {
    fprintf(stderr, "Read failed\n");
    goto out2;
  }

  close(in_fd);
  close(out_fd);

  ret = 0;
out2:
  usbg_cleanup(s);

out1:
  return ret;
}
