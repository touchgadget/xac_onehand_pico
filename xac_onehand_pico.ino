/*********************************************************************
 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 Copyright (c) 2019 Ha Thach for Adafruit Industriesi
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

/*
MIT License

Copyright (c) 2024 touchgadgetdev@gmail.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef USE_TINYUSB_HOST
  #error This example requires usb stack configured as host in "Tools -> USB Stack -> Adafruit TinyUSB Host"
#endif

const bool DEBUG_ON = true;

// This option sends LOTS of debug data so do not enable unless
// absolutely needed.
#define DEBUG_HID_DATA (0)

#define DBG_begin(...)    if (DEBUG_ON) Serial1.begin(__VA_ARGS__)
#define DBG_print(...)    if (DEBUG_ON) Serial1.print(__VA_ARGS__)
#define DBG_println(...)  if (DEBUG_ON) Serial1.println(__VA_ARGS__)
#define DBG_printf(...)   if (DEBUG_ON) Serial1.printf(__VA_ARGS__)

#include "Adafruit_TinyUSB.h"
#include <Wire.h>

Adafruit_USBH_Host USBHost;

// Flight sim joystick HID report layout
// Large joystick X, Y, Z (twist) axes
// 8 way hat switch
// 12 buttons (6 on the stick, 6 on the base)
// throttle slider
typedef struct __attribute__ ((packed)) {
  uint32_t x : 10;      // 0..512..1023
  uint32_t y : 10;      // 0..512..1023
  uint32_t hat : 4;
  uint32_t twist : 8;   // 0..127..255
  uint8_t buttons_a;
  uint8_t slider;       // 0..255
  uint8_t buttons_b;
} FSJoystick_Report_t;

// I2C flight stick joystick
// Thrustmaster T.16000M flight joystick *******************************
const uint16_t T16K_VID = 0x044f;
const uint16_t T16K_PID = 0xb10a;
#define isT16K(vid, pid)  ((vid == T16K_VID) && (pid == T16K_PID))
bool T16K = false;
// Thrustmaster T.16000M Flight sim joystick HID report layout
// Large joystick X, Y, Z (twist) axes
// 8 way hat switch
// 16 buttons (4 on the stick, 12 on the base)
// throttle slider
typedef struct __attribute__ ((packed)) {
  uint16_t	buttons;
  uint8_t		hat:4;
  uint8_t		filler:4;
  uint16_t	x;
  uint16_t	y;
  uint8_t		twist;
  uint8_t		slider;
} T16K_Report_t;

// Logitech Extreme 3D Pro flight joystick *****************************
const uint16_t LE3DP_VID = 0x046d;
const uint16_t LE3DP_PID = 0xc215;
#define isLE3DP(vid, pid)  ((vid == LE3DP_VID) && (pid == LE3DP_PID))
bool LE3DP = false;
// Flight sim joystick HID report layout
// Large joystick X, Y, Z (twist) axes
// 8 way hat switch
// 12 buttons (6 on the stick, 6 on the base)
// throttle slider
typedef struct __attribute__ ((packed)) {
  uint32_t x : 10;      // 0..512..1023
  uint32_t y : 10;      // 0..512..1023
  uint32_t hat : 4;
  uint32_t twist : 8;   // 0..127..255
  uint8_t buttons_a;
  uint8_t slider;       // 0..255
  uint8_t buttons_b;
} LE3DP_Report_t;

// Logitech X52 H.O.T.A.S **********************************************
const uint16_t LX52_VID = 0x06a3;
const uint16_t LX52_PID = 0x075c;
#define isLX52(vid, pid)  ((vid == LX52_VID) && (pid == LX52_PID))
bool LX52 = false;
// Flight sim joystick HID report layout
// Large joystick X, Y, Z (twist) axes
// 3 X 8 way hat switches
// 34 buttons
// throttle slider
typedef struct __attribute__ ((packed)) {
  uint16_t x:11;
  uint16_t y:11;
  uint16_t twist:10;
  uint8_t z;
  uint8_t rx;
  uint8_t ry;
  uint8_t slider;
  uint64_t buttons:34;
  uint8_t filler:2;
  uint8_t dpad:4;
  uint8_t dpad2:4;
  uint8_t dpad3:4;
} LX52_Report_t;

const uint16_t XAC_MAX = 1023;
const uint16_t XAC_MID = 511;
const uint16_t XAC_MIN = 0;

const uint16_t HAT2XAC[16] = {
  XAC_MIN,  // North
  XAC_MIN,  // North East
  XAC_MID,  // East
  XAC_MAX,  // South East
  XAC_MAX,  // South
  XAC_MAX,  // South West
  XAC_MID,  // West
  XAC_MIN,  // North West
  XAC_MID,  // No direction
  XAC_MID,  // No direction
  XAC_MID,  // No direction
  XAC_MID,  // No direction
  XAC_MID,  // No direction
  XAC_MID,  // No direction
  XAC_MID,  // No direction
  XAC_MID   // No direction
};

void xac_center_all(void) {
  FSJoystick_Report_t FSJoy_left, FSJoy_right;

  // I2C left joystick
  memset(&FSJoy_left, 0, sizeof(FSJoy_left));
  FSJoy_left.x = XAC_MID;
  FSJoy_left.y = XAC_MID;
  FSJoy_left.twist = 127;
  Wire.beginTransmission(0x30);
  Wire.write((uint8_t *)&FSJoy_left, sizeof(FSJoy_left));
  Wire.endTransmission();

  // I2C right joystick
  memset(&FSJoy_right, 0, sizeof(FSJoy_right));
  FSJoy_right.x = XAC_MID;
  FSJoy_right.y = XAC_MID;
  FSJoy_right.twist = 127;
  Wire1.beginTransmission(0x30);
  Wire1.write((uint8_t *)&FSJoy_right, sizeof(FSJoy_right));
  Wire1.endTransmission();
}


//--------------------------------------------------------------------+
// Setup and Loop on Core0
//--------------------------------------------------------------------+

void setup()
{
  Wire.setClock(400000);
  Wire.begin();
  Wire1.setClock(400000);
  Wire1.begin();
  
  if (DEBUG_ON) {
    // wait until device mounted
    DBG_begin(115200);
    DBG_println("Flight stick/gamepad to two USB joysticks");
  }

  USBHost.begin(0);

  xac_center_all();
}

typedef struct {
  uint8_t report[64];
  uint32_t report_count = 0;
  uint32_t available_count = 0;
  uint32_t last_millis = 0;
  uint8_t len;
  bool available = false;
  bool skip_report_id;
  bool hid_joystick;
} HID_state_t;

volatile HID_state_t HID_Report;

void loop()
{
  USBHost.task();
  if (HID_Report.available) {
    if (HID_Report.len > 2) {
      FSJoystick_Report_t FSJoy_left, FSJoy_right;
      memset(&FSJoy_left, 0, sizeof(FSJoy_left));
      memset(&FSJoy_right, 0, sizeof(FSJoy_right));

      if (LE3DP && (HID_Report.len == sizeof(LE3DP_Report_t))) {
        LE3DP_Report_t *rpt = (LE3DP_Report_t *)HID_Report.report;
        FSJoy_left.x = map(rpt->x, 0, 1023, XAC_MIN, XAC_MAX);
        FSJoy_left.y = map(rpt->y, 0, 1023, XAC_MIN, XAC_MAX);
        FSJoy_left.buttons_a = rpt->buttons_a;
        FSJoy_right.x = map(rpt->twist, 0, 255, XAC_MIN, XAC_MAX);
        FSJoy_right.y = HAT2XAC[rpt->hat];
        FSJoy_right.buttons_a = rpt->buttons_b;
#if DEBUG_HID_DATA
        DBG_printf("LE3DP x: %d xac x: %d y: %d xac y: %d "
            "LE3DP twist: %d FSJoy_right.x: %d LE3DP hat: %d FSJoy_right.y: %d\r\n",
            rpt->x, FSJoy_left.x, rpt->y, FSJoy_left.y,
            rpt->twist, FSJoy_right.x, rpt->hat, FSJoy_right.y);
#endif
      } if (LX52 && (HID_Report.len == sizeof(LX52_Report_t))) {
        LX52_Report_t *rpt = (LX52_Report_t *)HID_Report.report;
        FSJoy_left.x = map(rpt->x, 0, 2047, XAC_MIN, XAC_MAX);
        FSJoy_left.y = map(rpt->y, 0, 2047, XAC_MIN, XAC_MAX);
        FSJoy_left.buttons_a = rpt->buttons & 0xFF;
        FSJoy_right.x = map(rpt->twist, 0, 1023, XAC_MIN, XAC_MAX);
        FSJoy_right.y = (rpt->dpad == 0) ? XAC_MID : HAT2XAC[rpt->dpad - 1];
        FSJoy_right.buttons_a = (rpt->buttons >> 8) & 0xFF;
#if DEBUG_HID_DATA
        DBG_printf("LX52 x: %d xac x: %d y: %d xac y: %d "
            "LX52 twist: %d FSJoy_right.x: %d LX52 hat: %d FSJoy_right.y: %d\r\n",
            rpt->x, FSJoy_left.x, rpt->y, FSJoy_left.y,
            rpt->twist, FSJoy_right.x, rpt->dpad, FSJoy_right.y);
#endif
      } if (T16K && (HID_Report.len == sizeof(T16K_Report_t))) {
        T16K_Report_t *rpt = (T16K_Report_t *)HID_Report.report;
        FSJoy_left.x = map(rpt->x, 0, 16383, XAC_MIN, XAC_MAX);
        FSJoy_left.y = map(rpt->y, 0, 16383, XAC_MIN, XAC_MAX);
        FSJoy_left.buttons_a = rpt->buttons & 0xFF;
        FSJoy_right.x = map(rpt->twist, 0, 255, XAC_MIN, XAC_MAX);
        FSJoy_right.y = HAT2XAC[rpt->hat];
        FSJoy_right.buttons_a = (rpt->buttons >> 8) & 0xFF;
#if DEBUG_HID_DATA
        DBG_printf("T16K x: %d xac x: %d y: %d xac y: %d "
            "T16K twist: %d FSJoy_right.x: %d T16K hat: %d FSJoy_right.y: %d\r\n",
            rpt->x, FSJoy_left.x, rpt->y, FSJoy_left.y,
            rpt->twist, FSJoy_right.x, rpt->hat, FSJoy_right.y);
#endif
      }
      // I2C left joystick
      Wire.beginTransmission(0x30);
      Wire.write((uint8_t *)&FSJoy_left, sizeof(FSJoy_left));
      Wire.endTransmission();

      // I2C right joystick
      Wire1.beginTransmission(0x30);
      Wire1.write((uint8_t *)&FSJoy_right, sizeof(FSJoy_right));
      Wire1.endTransmission();
    }
    HID_Report.available = false;
  }
}

// Invoked when device with hid interface is mounted
// Report descriptor is also available for use.
// tuh_hid_parse_report_descriptor() can be used to parse common/simple enough
// descriptor. Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE,
// it will be skipped therefore report_desc = NULL, desc_len = 0
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *desc_report, uint16_t desc_len) {
  uint16_t vid, pid;
  tuh_vid_pid_get(dev_addr, &vid, &pid);

  DBG_printf("HID device address = %d, instance = %d is mounted\r\n", dev_addr, instance);
  DBG_printf("VID = %04x, PID = %04x\r\n", vid, pid);
  LE3DP = isLE3DP(vid, pid);
  LX52 = isLX52(vid, pid);
  T16K = isT16K(vid, pid);
  uint8_t const protocol_mode = tuh_hid_get_protocol(dev_addr, instance);
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);
  DBG_printf("protocol_mode=%d,itf_protocol=%d\r\n",
      protocol_mode, itf_protocol);

  const size_t REPORT_INFO_MAX = 8;
  tuh_hid_report_info_t report_info[REPORT_INFO_MAX];
  uint8_t report_num = tuh_hid_parse_report_descriptor(report_info,
      REPORT_INFO_MAX, desc_report, desc_len);
  DBG_printf("HID descriptor reports:%d\r\n", report_num);
  for (size_t i = 0; i < report_num; i++) {
    DBG_printf("%d,%d,%d\r\n", report_info[i].report_id, report_info[i].usage,
        report_info[i].usage_page);
    HID_Report.skip_report_id = false;
    HID_Report.hid_joystick = false;
    if ((report_info[i].usage_page == 1) && (report_info[i].usage == 0)) {
      HID_Report.hid_joystick = true;
    }
  }

  if (desc_report && desc_len) {
    for (size_t i = 0; i < desc_len; i++) {
      DBG_printf("%x,", desc_report[i]);
      if ((i & 0x0F) == 0x0F) DBG_println();
    }
    DBG_println();
  }

  HID_Report.report_count = 0;
  HID_Report.available_count = 0;
  DBG_printf("Start polling instance %d\n", instance);
  if (!tuh_hid_receive_report(dev_addr, instance)) {
    DBG_println("Error: cannot request to receive report");
  }
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
  DBG_printf("HID device address = %d, instance = %d is unmounted\r\n", dev_addr, instance);
}

// Invoked when received report from device via interrupt endpoint
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance,
    uint8_t const *report, uint16_t len) {
  HID_Report.report_count++;
  if (HID_Report.available) {
    static uint32_t dropped = 0;
    DBG_printf("drops=%lu\r\n", ++dropped);
  } else {
    if (HID_Report.skip_report_id) {
      // Skip first byte which is report ID.
      report++;
      len--;
    }
    memcpy((void *)HID_Report.report, report, min(len, sizeof(HID_Report.report)));
    HID_Report.len = len;
    HID_Report.available = true;
    HID_Report.available_count++;
    HID_Report.last_millis = millis();
  }
  // continue to request to receive report
  if (!tuh_hid_receive_report(dev_addr, instance)) {
    DBG_println("Error: cannot request to receive report");
  }
}
