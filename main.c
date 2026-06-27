// SPDX-License-Identifier: GPL-3.0-or-later
#include "libevdev-1.0/libevdev/libevdev.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/hidraw.h>
#include <linux/input.h>
#include <linux/usb/ch9.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <usbg/function/hid.h>
#include <usbg/usbg.h>

#define USBG_VENDOR 0x2d1f
#define USBG_PRODUCT 0x0095
#define W9013_VENDOR 0x2d1f
#define W9013_PRODUCT 0x0095

#define WS8100_PEN_NAME "ws8100_pen"
#define CYTTSP5_NAME "cyttsp5"
#define W9013_NAME "w9013 2D1F:0095 Stylus"

static char report_desc_w9013[] = {
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
    0xc0,             // End Collection
};

static char report_desc_touch[] = {
    0x05, 0x0d,       // Usage Page (Digitizers)
    0x09, 0x04,       // Usage (Touch Screen) // change to 05 for touchpad
    0xa1, 0x01,       // Collection (Application)
    0x85, 0x01,       //   Report ID (1)
    0x09, 0x22,       //   Usage (Finger)
    0xa1, 0x02,       //   Collection (Logical)
    0x09, 0x42,       //     Usage (Tip Switch)
    0x15, 0x00,       //     Logical Minimum (0)
    0x25, 0x01,       //     Logical Maximum (1)
    0x75, 0x01,       //     Report Size (1)
    0x95, 0x01,       //     Report Count (1)
    0x81, 0x02,       //     Input (Data,Var,Abs)
    0x75, 0x01,       //     Report Size (1)
    0x95, 0x03,       //     Report Count (3)
    0x81, 0x03,       //     Input (Cnst,Var,Abs)
    0x25, 0x0f,       //     Logical Maximum (15)
    0x75, 0x04,       //     Report Size (4)
    0x95, 0x01,       //     Report Count (1)
    0x09, 0x51,       //     Usage (Contact Id)
    0x81, 0x02,       //     Input (Data,Var,Abs)
    0x05, 0x01,       //     Usage Page (Generic Desktop)
    0x15, 0x00,       //     Logical Minimum (0)
    0x26, 0x46, 0x07, //     Logical Maximum (1862)
    0x75, 0x10,       //     Report Size (16)
    0x55, 0x0e,       //     Unit Exponent (-2)
    0x65, 0x11,       //     Unit (SILinear: cm)
    0x09, 0x30,       //     Usage (X)
    0x35, 0x00,       //     Physical Minimum (0)
    0x46, 0x46, 0x07, //     Physical Maximum (1862)
    0x95, 0x01,       //     Report Count (1)
    0x81, 0x02,       //     Input (Data,Var,Abs)
    0x26, 0x76, 0x05, //     Logical Maximum (1398)
    0x09, 0x31,       //     Usage (Y)
    0x46, 0x76, 0x05, //     Physical Maximum (1398)
    0x81, 0x02,       //     Input (Data,Var,Abs)
    0xc0,             //   End Collection
    0x05, 0x0d,       //   Usage Page (Digitizers)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x7f,       //   Logical Maximum (127)
    0x75, 0x08,       //   Report Size (8)
    0x95, 0x01,       //   Report Count (1)
    0x09, 0x54,       //   Usage (Contact Count)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x55, 0x0c,       //   Unit Exponent (-4)
    0x66, 0x01, 0x10, //   Unit (SI Linear: s)
    0x27, 0xff, 0xff, 0x00, 0x00, //   Logical Maximum (65535)
    0x47, 0xff, 0xff, 0x00, 0x00, //   Physical Maximum (65535)
    0x75, 0x10,                   //   Report Size (16)
    0x95, 0x01,                   //   Report Count (1)
    0x09, 0x56,                   //   Usage (Scan Time)
    0x81, 0x02,                   //   Input (Data,Var,Abs)
    0xc0                          // End Collection
};

#define MAX_SLOTS 10

typedef struct {
  uint16_t x[MAX_SLOTS];
  uint16_t y[MAX_SLOTS];
  int32_t tid[MAX_SLOTS];
  bool lifted[MAX_SLOTS];
  uint8_t current;
} slots;

typedef struct {
  usbg_state *s;
  usbg_gadget *g;
  usbg_config *c;
  usbg_function *f_hid_w9013;
  usbg_function *f_hid_touch;
} usbg_context;

int remove_gadget(usbg_gadget *g) {
  int usbg_ret;
  usbg_udc *u;

  /* Check if gadget is enabled */
  u = usbg_get_gadget_udc(g);

  /* If gadget is enable we have to disable it first */
  if (u) {
    usbg_ret = usbg_disable_gadget(g);
    if (usbg_ret != USBG_SUCCESS) {
      fprintf(stderr, "Error on USB disable gadget udc\n");
      fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
              usbg_strerror(usbg_ret));
      goto out;
    }
  }

  /* Remove gadget with USBG_RM_RECURSE flag to remove
   * also its configurations, functions and strings */
  usbg_ret = usbg_rm_gadget(g, USBG_RM_RECURSE);
  if (usbg_ret != USBG_SUCCESS) {
    fprintf(stderr, "Error on USB gadget remove\n");
    fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
            usbg_strerror(usbg_ret));
  }

out:
  return usbg_ret;
}

int initUSB(usbg_context *usb_ctx, bool use_cyttsp5, uint16_t vendor,
            uint16_t product) {
  int usbg_ret = -EINVAL;

  usbg_gadget *old_gadget = NULL;
  struct usbg_gadget_attrs g_attrs = {
      .bcdUSB = 0x0200,
      .bDeviceClass = USB_CLASS_PER_INTERFACE,
      .bDeviceSubClass = 0x00,
      .bDeviceProtocol = 0x00,
      .bMaxPacketSize0 = 64, /* Max allowed ep0 packet size */
      .idVendor = vendor,
      .idProduct = product,
      .bcdDevice = 0x0100, /* Verson of device */
  };
  struct usbg_gadget_strs g_strs = {
      .serial = "fedcba9876543210", /* Serial number */
      .manufacturer = "Pine64",     /* Manufacturer */
      .product = "PineNote"         /* Product string */
  };
  struct usbg_config_strs c_strs = {.configuration = "2xHID"};
  struct usbg_f_hid_attrs f_attrs_ws9013 = {
      .protocol = 0,
      .report_desc =
          {
              .desc = report_desc_w9013,
              .len = sizeof(report_desc_w9013),
          },
      .report_length = 15,
      .subclass = 0,
  };
  struct usbg_f_hid_attrs f_attrs_touch = {
      .protocol = 0,
      .report_desc =
          {
              .desc = report_desc_touch,
              .len = sizeof(report_desc_touch),
          },
      .report_length = 9,
      .subclass = 0,
  };

  usbg_ret = usbg_init("/sys/kernel/config", &usb_ctx->s);
  if (usbg_ret != USBG_SUCCESS) {
    fprintf(stderr, "Error on usbg init\n");
    fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
            usbg_strerror(usbg_ret));
    goto out1;
  }

  old_gadget = usbg_get_gadget(usb_ctx->s, "pinenote-usb-tablet");
  if (old_gadget) {
    fprintf(stderr, "Removing leftover gadget\n");
    usbg_ret = remove_gadget(old_gadget);
    if (usbg_ret != USBG_SUCCESS)
      goto out2;
  }

  usbg_ret = usbg_create_gadget(usb_ctx->s, "pinenote-usb-tablet", &g_attrs,
                                &g_strs, &usb_ctx->g);
  if (usbg_ret != USBG_SUCCESS) {
    fprintf(stderr, "Error creating gadget\n");
    fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
            usbg_strerror(usbg_ret));
    goto out2;
  }
  usbg_ret = usbg_create_function(usb_ctx->g, USBG_F_HID, "usb0",
                                  &f_attrs_ws9013, &usb_ctx->f_hid_w9013);
  if (usbg_ret != USBG_SUCCESS) {
    fprintf(stderr, "Error creating function\n");
    fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
            usbg_strerror(usbg_ret));
    goto out2;
  }
  if (use_cyttsp5) {
    usbg_ret = usbg_create_function(usb_ctx->g, USBG_F_HID, "usb1",
                                    &f_attrs_touch, &usb_ctx->f_hid_touch);
    if (usbg_ret != USBG_SUCCESS) {
      fprintf(stderr, "Error creating function\n");
      fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
              usbg_strerror(usbg_ret));
      goto out2;
    }
  }
  usbg_ret = usbg_create_config(usb_ctx->g, 1, "The only one", NULL, &c_strs,
                                &usb_ctx->c);
  if (usbg_ret != USBG_SUCCESS) {
    fprintf(stderr, "Error creating config\n");
    fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
            usbg_strerror(usbg_ret));
    goto out2;
  }
  usbg_ret =
      usbg_add_config_function(usb_ctx->c, "w9013", usb_ctx->f_hid_w9013);
  if (usbg_ret != USBG_SUCCESS) {
    fprintf(stderr, "Error adding function\n");
    fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
            usbg_strerror(usbg_ret));
    goto out2;
  }
  if (use_cyttsp5) {
    usbg_ret =
        usbg_add_config_function(usb_ctx->c, "touch", usb_ctx->f_hid_touch);
    if (usbg_ret != USBG_SUCCESS) {
      fprintf(stderr, "Error adding function\n");
      fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
              usbg_strerror(usbg_ret));
      goto out2;
    }
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

int handle_ws8100_pen_events(struct input_event ev, void *data,
                             pthread_mutex_t *out_mutex, int out_fd) {
  unsigned char *buttons = data;
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
      pthread_mutex_lock(out_mutex);
      if (write(out_fd, buttons, 2) != 2 && errno != ESHUTDOWN) {
        perror("Write failed");
        pthread_mutex_unlock(out_mutex);
        return -1;
      }
      pthread_mutex_unlock(out_mutex);
    }
  }
  return 0;
}

int handle_cyttsp_events(struct input_event ev, void *data,
                         pthread_mutex_t *out_mutex, int out_fd) {
  slots *touches = data;
  if (ev.type == EV_ABS) {
    switch (ev.code) {
    case ABS_MT_SLOT:
      touches->current = ev.value;
      break;
    case ABS_MT_TRACKING_ID:
      touches->tid[touches->current] = ev.value;
      break;
    case ABS_MT_POSITION_X:
      touches->x[touches->current] = ev.value;
      break;
    case ABS_MT_POSITION_Y:
      touches->y[touches->current] = ev.value;
      break;
    }
  } else if (ev.type == EV_SYN && ev.code == SYN_REPORT) {
    bool active[MAX_SLOTS] = {0};
    bool lifting[MAX_SLOTS] = {0};
    uint8_t n_touches = 0;

    for (int i = 0; i < MAX_SLOTS; i++) {
      if (touches->tid[i] != -1) {
        active[i] = true;
        n_touches++;
      } else if (touches->lifted[i] == false) {
        lifting[i] = true;
        n_touches++;
      }
    }

    for (int i = 0; i < MAX_SLOTS; i++) {
      if (!(active[i] || lifting[i]))
        continue;

      uint8_t report[9] = {0x01, (active[i] ? 0x01 : 0x00) | ((i & 0x0F) << 4)};
      memcpy(&report[2], &touches->x[i], 2);
      memcpy(&report[4], &touches->y[i], 2);
      report[6] = n_touches;
      uint16_t time = ev.time.tv_usec / 100 + ev.time.tv_sec * 10000;
      memcpy(&report[7], &time, 2);

      if (write(out_fd, report, 9) != 9 && errno != ESHUTDOWN) {
        perror("Write failed");
        return -1;
      }
    }

    for (int i = 0; i < MAX_SLOTS; i++) {
      touches->lifted[i] = touches->tid[i] == -1;
    }
  }
  return 0;
}

volatile sig_atomic_t keepRunning = true;
static int wakeup_pipe_write;

void intHandler(int dummy) {
  keepRunning = false;
  write(wakeup_pipe_write, "\0", 1);
}

typedef int (*evdev_handler_fn)(struct input_event, void *data,
                                pthread_mutex_t *mutex, int out_fd);

typedef struct {
  struct libevdev *dev;
  int fd;
  void *data;
  pthread_mutex_t *out_mutex;
  int out_fd;
  int wakeup_r;
  int wakeup_w;
  evdev_handler_fn handler;
} evdev_worker_args;

void *evdev_worker(void *arg) {
  evdev_worker_args *a = arg;
  int evdev_rc;
  struct pollfd fds[] = {
      {.fd = a->fd, .events = POLLIN},
      {.fd = a->wakeup_r, .events = POLLIN},
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
        evdev_rc = libevdev_next_event(a->dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
        if (evdev_rc == LIBEVDEV_READ_STATUS_SYNC) {
          printf("dropped\n");
          while (evdev_rc == LIBEVDEV_READ_STATUS_SYNC) {
            evdev_rc =
                libevdev_next_event(a->dev, LIBEVDEV_READ_FLAG_SYNC, &ev);
            if (a->handler(ev, a->data, a->out_mutex, a->out_fd) < 0)
              goto exit;
          }
          printf("re-synced\n");
        } else if (evdev_rc == LIBEVDEV_READ_STATUS_SUCCESS) {
          if (a->handler(ev, a->data, a->out_mutex, a->out_fd) < 0)
            goto exit;
        } else {
          fprintf(stderr, "Failed to handle events: %s\n", strerror(-evdev_rc));
          goto exit;
        }
      } while (libevdev_has_event_pending(a->dev));
    }
  }
exit:
  keepRunning = false;
  write(a->wakeup_w, "\0", 1);
  return NULL;
}

int main(int argc, char *argv[]) {
  int wakeup_pipe[2];
  bool use_cyttsp5 = false, grab_cyttsp5 = false;
  int w9013, out_fd, out_fd2, evdev_rc, ws8100_pen_fd, cyttsp5_fd;
  unsigned char w9013_buffer[15];
  ssize_t bytes = 0;
  unsigned char buttons[2] = {1, 0};
  slots *cyttsp5_touches;
  usbg_context usb_ctx = {0};
  struct libevdev *ws8100_pen, *cyttsp5, *w9013_evdev = NULL;
  pthread_mutex_t out_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_t cyttsp5_thread;
  pthread_t ws8100_pen_thread;
  evdev_worker_args cyttsp5_args, ws8100_pen_args;
  uint16_t vendor = USBG_VENDOR;
  uint16_t product = USBG_PRODUCT;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--use-touchscreen") == 0) {
      use_cyttsp5 = true;
      grab_cyttsp5 = true;
    } else if (strcmp(argv[i], "--grab-touchscreen") == 0) {
      grab_cyttsp5 = true;
    } else if (strcmp(argv[i], "--vendor") == 0 && i + 1 < argc) {
      vendor = (uint16_t)strtoul(argv[++i], NULL, 16);
    } else if (strcmp(argv[i], "--product") == 0 && i + 1 < argc) {
      product = (uint16_t)strtoul(argv[++i], NULL, 16);
    } else {
      printf("grabs and forwards PineNote's stylus and (optionally) "
             "touchscreen input.\n");
      printf("Usage: %s [options]\n\n", argv[0]);
      printf("Options:\n"
             "  --use-touchscreen   grab and forward touchscreen input\n"
             "  --grab-touchscreen  grab touchscreen input\n");
      return -1;
    }
  }

  if (initUSB(&usb_ctx, use_cyttsp5, vendor, product) < 0) {
    fprintf(stderr, "Failed to init usb gadget");
    goto cleanup_usb;
  }

  out_fd = open("/dev/hidg0", O_WRONLY);
  if (out_fd < 0) {
    perror("Failed to open /dev/hidg0");
    goto cleanup_usb;
  }

  if (use_cyttsp5) {
    out_fd2 = open("/dev/hidg1", O_WRONLY);
    if (out_fd2 < 0) {
      perror("Failed to open /dev/hidg1");
      goto cleanup_usb;
    }
  }

  w9013 = find_hidraw_device("w9013 digitizer", W9013_VENDOR, W9013_PRODUCT);
  if (w9013 < 0) {
    fprintf(stderr, "Failed to find w9013 digitizer");
    goto cleanup_out_fd;
  }

  evdev_rc = find_evdev_device(W9013_NAME, &w9013_evdev);
  if (evdev_rc < 0) {
    fprintf(stderr, "Failed to find w9013");
    goto cleanup_w9013;
  }
  evdev_rc = libevdev_grab(w9013_evdev, LIBEVDEV_GRAB);
  if (evdev_rc < 0) {
    fprintf(stderr, "Failed to grab w9013");
    goto cleanup_w9013_evdev;
  }

  evdev_rc = find_evdev_device(WS8100_PEN_NAME, &ws8100_pen);
  if (evdev_rc < 0) {
    fprintf(stderr, "Failed to find ws8100_pen");
    goto cleanup_w9013_evdev;
  }
  ws8100_pen_fd = libevdev_get_fd(ws8100_pen);
  evdev_rc = libevdev_grab(ws8100_pen, LIBEVDEV_GRAB);
  if (evdev_rc < 0) {
    fprintf(stderr, "Failed to grab ws8100_pen");
    goto cleanup_ws8100;
  }

  if (grab_cyttsp5) {
    if (!(cyttsp5_touches = calloc(1, sizeof(slots))))
      goto cleanup_ws8100;
    for (int i = 0; i < MAX_SLOTS; i++) {
      cyttsp5_touches->tid[i] = -1;
    }
    evdev_rc = find_evdev_device(CYTTSP5_NAME, &cyttsp5);
    if (evdev_rc < 0) {
      fprintf(stderr, "Failed to find cyttsp5");
      free(cyttsp5_touches);
      goto cleanup_ws8100;
    }
    cyttsp5_fd = libevdev_get_fd(cyttsp5);
    evdev_rc = libevdev_grab(cyttsp5, LIBEVDEV_GRAB);
    if (evdev_rc < 0) {
      fprintf(stderr, "Failed to grab cyttsp5");
      goto cleanup_all;
    }
  }

  pipe(wakeup_pipe);
  wakeup_pipe_write = wakeup_pipe[1];
  signal(SIGINT, intHandler);

  if (use_cyttsp5) {
    cyttsp5_args = (evdev_worker_args){.dev = cyttsp5,
                                       .fd = cyttsp5_fd,
                                       .data = cyttsp5_touches,
                                       .out_mutex = &out_mutex,
                                       .out_fd = out_fd2,
                                       .wakeup_r = wakeup_pipe[0],
                                       .wakeup_w = wakeup_pipe[1],
                                       .handler = handle_cyttsp_events};
    pthread_create(&cyttsp5_thread, NULL, evdev_worker, &cyttsp5_args);
  }

  ws8100_pen_args = (evdev_worker_args){.dev = ws8100_pen,
                                        .fd = ws8100_pen_fd,
                                        .data = buttons,
                                        .out_mutex = &out_mutex,
                                        .out_fd = out_fd,
                                        .wakeup_r = wakeup_pipe[0],
                                        .wakeup_w = wakeup_pipe[1],
                                        .handler = handle_ws8100_pen_events};
  pthread_create(&ws8100_pen_thread, NULL, evdev_worker, &ws8100_pen_args);

  struct pollfd fds[] = {
      {.fd = w9013, .events = POLLIN},
      {.fd = wakeup_pipe[0], .events = POLLIN},
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
      while ((bytes = read(w9013, w9013_buffer, sizeof(w9013_buffer))) > 0) {
        pthread_mutex_lock(&out_mutex);
        if (write(out_fd, w9013_buffer, bytes) != bytes && errno != ESHUTDOWN) {
          perror("Write failed");
          pthread_mutex_unlock(&out_mutex);
          goto exit;
        }
        pthread_mutex_unlock(&out_mutex);
      }
      if (bytes < 0 && errno != EAGAIN) {
        perror("Read failed");
        goto exit;
      }
    }
  }
exit:
  keepRunning = 0;
  write(wakeup_pipe[1], "\0", 1);
  if (use_cyttsp5) {
    pthread_join(cyttsp5_thread, NULL);
  }
  pthread_join(ws8100_pen_thread, NULL);
cleanup_all:
  if (grab_cyttsp5) {
    libevdev_grab(cyttsp5, LIBEVDEV_UNGRAB);
    libevdev_free(cyttsp5);
    close(cyttsp5_fd);
    free(cyttsp5_touches);
  }
cleanup_ws8100:
  libevdev_grab(ws8100_pen, LIBEVDEV_UNGRAB);
  libevdev_free(ws8100_pen);
  close(ws8100_pen_fd);
cleanup_w9013_evdev:
  libevdev_grab(w9013_evdev, LIBEVDEV_UNGRAB);
  libevdev_free(w9013_evdev);
cleanup_w9013:
  close(w9013);
cleanup_out_fd:
  close(out_fd);
cleanup_usb:
  cleanupUSB(&usb_ctx);
  return 0;
}
