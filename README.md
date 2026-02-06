[English](https://github.com/barryblueice/ESP32-Precision-TouchPad/blob/main/README.md) | [Simplified Chinese](https://github.com/barryblueice/ESP32-Precision-TouchPad/blob/main/README_CN.md)

<img width="900" height="723" alt="image" src="https://github.com/user-attachments/assets/6dd84c6f-8089-4962-9cd4-7cbeaad9e661" />

# **What's This?**

A touchpad peripheral device based on ESP32-S2 + specific laptop TouchPad. 

 - Compatible with Microsoft Precision TouchPad (PTP) standard. 
 - Supported Windows Touch Gesture. 
 - Supported Taptic Engine.
   > Taptic Engine still in development.
 
Also include a HP fingerprint module, in order to support fingerprint for Windows Hello.

### **Currently Touchpad Support (already tested):**
 - Lenovo ELAN 33370A TouchPad (Rev.A S8974A, traditional mechanical design)
 - XiaoMi book 14/16 pro Goodix TouchPad (Taptic Engine)

### **Hardware OpenSource Link：[ESP32-Precision-TouchPad](https://oshwhub.com/barryblueice/esp32-precision-touchpad)**

> [!CAUTION]
> Hardware & software only support the models of the touchpad mentioned above, untested touchpad models may be incompatible. Use of such models can lead to driver failure or permanent hardware damage caused by short circuits!

> [!IMPORTANT]  
> The pressure-sensitive touchpad promoted by Xiaomi merely replaced the traditional mechanical structure with a pressure-sensitive motor trigger. It is not a genuine pressure-sensing recognition (there is no pressure-sensing data).

#### **PTP Mode Demonstration**:

https://github.com/user-attachments/assets/256ca4d0-b585-47dc-b8cc-44001de3028a

#### **Mouse Mode Demonstration**:

https://github.com/user-attachments/assets/cb5ae0a1-41e6-4b15-b257-127867edca49

#### **Android Demonstration**:

https://github.com/user-attachments/assets/dce2b15a-eb64-4c0a-b503-1ea32e15b02b

#### **Application:**

 - **TouchPad Main PCB:**

<img width="900" height="667" alt="image" src="https://github.com/user-attachments/assets/a25d1b9c-eece-432a-9ca9-677dc87e56ad" />

<img width="900" height="623" alt="image" src="https://github.com/user-attachments/assets/813ec446-ebc3-422f-ba02-ab4217bdd4ef" />

 - **2.4G Reciever:**

<img width="900" height="517" alt="image" src="https://github.com/user-attachments/assets/2af38202-edb0-4c51-ac50-329084c07bd0" />

<img width="900" height="504" alt="image" src="https://github.com/user-attachments/assets/e1ca4a4e-5156-40c2-ac76-7019ce21d6cf" />

 - **Appearance Design (modeling by SOLIDWORKS):**

<img width="900" height="506" alt="image" src="https://github.com/user-attachments/assets/42be22d2-275e-4799-bdfd-6f022412048f" />

<img width="900" height="506" alt="image" src="https://github.com/user-attachments/assets/bb75f841-200c-4c47-9cbe-ed900720e240" />

<img width="900" height="506" alt="image" src="https://github.com/user-attachments/assets/28aea2f9-f43a-4217-a47e-e8b8bca49d43" />

 - **System Application:**

<img width="501" height="500" alt="image" src="https://github.com/user-attachments/assets/ca5a84f5-3594-45f0-bfe8-55217592828e" />

<img width="536" height="182" alt="image" src="https://github.com/user-attachments/assets/9c88e2ca-030a-46b5-9981-c7615a9c184d" />

***

# TODO List:
  ## Hardware:
  - [x] PCB Design
  - [x] Material Selection
  - [x] Appearance Design (Modeling by SOLIDWORKS)
  - [x] Adding battery to support 2.4G wireless (Fingerprint module is unavailable during 2.4G wireless state)
  - [x] Adding battery to support wireless Bluetooth (Fingerprint module is unavailable during bluetooth wireless state)
  - [ ] ~~Adding ThinkPad TrackPoint Mouse Pointer~~ (There are no current plans for this function update)
  - [ ] ~~Adding specific function button~~ (There are no current plans for this function update)

  ## Software:
  - [x] Recognized as a Microsoft Precision TouchPad
  - [x] Driving ELAN TouchPad
  - [x] **Switching From Mouse Mode (Only One Finger) to Absolute Mode (Supported Multi Finger), [thanks to @ApprehensiveAnt9858](https://www.reddit.com/r/embedded/comments/1j3c2k6/need_help_getting_and_i2c_hid_elan_touchpad_to/)**
  - [x] Single Touch Support
  - [x] Multi Touch Support
    - [x] Scroll Gesture Support
    - [x] Single Tap Support
    - [x] Multi Tap Support (It might fail to trigger occasionally)
  - [x] Physical Buttons Support (left click & right click)
  - [x] Changing from freertos polling to GPIO interrupt
  - [x] Add a new HID port to support Mouse Mode compatibility.<br>
        (available for some old systems/BIOS that doesn't support PTP, like Windows 7)
  - [x] 2.4G wireless mode development
  - [ ] Bluetooth wireleess mode development

***

# Current Issues:

 - Two fingers tap (equal to right click) can only trigger on laptop.<br>
   > Regarding the solution for triggering double-finger touch on a Desktop PC, please refer to [System Tuning Guidelines](https://github.com/barryblueice/ESP32-Precision-TouchPad/wiki/System%20Tuning%20Guidelines).
 - Multi-Port HID compatibility design may not support on some old system, like Windows XP.
 - Cannot perform rapid and consecutive clicks.
 - Due to processor limitation, Original PTP Mode can only support wire connect/2.4G wireless mode. <br>Bluetooth mode needs to use ESP32-S3.

***

# Current Support System (already tested):

 - Windows 7 (Mouse Mode), Windows 10/11 (PTP Mode);
 - Ubuntu 22.04 or newer (PTP Mode);
 - Oxygen OS 17 (based on Android 16) on PHK110 (PTP Mode);
 - HP / MSI BIOS (Mouse Mode)

***

# About Solution Implementation

Please go to [wiki page](https://github.com/barryblueice/ESP32-Precision-TouchPad/wiki) for further detail.

***

# ~~Related derivative projects:~~

 - ~~Still in development progress~~



