[English](https://github.com/barryblueice/ESP32-Precision-TouchPad/blob/main/README.md) | [Simplified Chineses](https://github.com/barryblueice/ESP32-Precision-TouchPad/blob/main/README_CN.md)

# **What's This?**

A touchpad peripheral device based on ESP32-S2 + ELAN TouchPad (Spare parts from Lenovo's inventory). Compatible with Microsoft Percision TouchPad (PTP) standard. Supported Windows Touch Gesture.
Also include a HP fingerprint module, in order to support fingerprint for Windows Hello.

### **Hardware OpenSource Link：[ESP32-Precision-TouchPad](https://oshwhub.com/barryblueice/esp32-precision-touchpad)**

#### **PTP Mode Demonstration**:

https://github.com/user-attachments/assets/256ca4d0-b585-47dc-b8cc-44001de3028a

#### **Mouse Mode Demonstration**:

https://github.com/user-attachments/assets/cb5ae0a1-41e6-4b15-b257-127867edca49

<img width="1194" height="970" alt="image" src="https://github.com/user-attachments/assets/35bba23a-5c70-4f4c-b9e3-434b141dd59f" />

<img width="826" height="823" alt="image" src="https://github.com/user-attachments/assets/ca5a84f5-3594-45f0-bfe8-55217592828e" />

<img width="536" height="182" alt="image" src="https://github.com/user-attachments/assets/9c88e2ca-030a-46b5-9981-c7615a9c184d" />

***

# TODO List:
  ## Hardware:
  - [x] PCB Design
  - [x] Material Selection
  - [ ] Appearance Design (Modeling by using SOLIDWORKS)
  - [ ] Adding battery to support Bluetooth (Fingerprint module is unavailable during bluetooth wireless state)
  - [ ] Adding ThinkPad TrackPoint Mouse Pointer
  - [ ] Adding specific function button

  ## Software:
  - [x] Recognized as a Microsoft Percision TouchPad
  - [x] Driving ELAN TouchPad
  - [x] **Switching From Mouse Mode (Only One Finger) to Absolute Mode (Supported Multi Finger), [thanks to @ApprehensiveAnt9858](https://www.reddit.com/r/embedded/comments/1j3c2k6/need_help_getting_and_i2c_hid_elan_touchpad_to/)**
  - [x] Single Touch Support
  - [x] Multi Touch Support
    - [x] Scroll Gesture Support
    - [x] Single Tap Support
    - [x] Multi Tap Support (It might fail to trigger occasionally)
  - [x] Physical Buttons Support (left click & right click)
  - [x] Changing from freertos polling to GPIO interrupt
  - [x] Add a new HID port to support Mouse Mode compatibility.
        (available for some old systems /BIOS that doesn't support PTP, like Windows 7)

***

# About Solution Implementation

Please go to [wiki page](https://github.com/barryblueice/ESP32-Precision-TouchPad/wiki) for further detail.

