# **What's This?**

A touchpad peripheral device based on ESP32-S2 + Lenovo ELAN TouchPad. Compatible with Microsoft Percision TouchPad (PTP) standard. Supported Windows Touch Gesture.
Also include a HP fingerprint module.

https://github.com/user-attachments/assets/b2fd1d32-9a8d-46fc-8a0d-ce17007ca7c2

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

  ## Software:
  - [x] Recognized as a Microsoft Percision TouchPad
  - [x] Driving ELAN TouchPad
  - [x] **Switching From Mouse Mode (Only One Finger) to Absolute Mode (Supported Multi Finger), [thanks to @ApprehensiveAnt9858](https://www.reddit.com/r/embedded/comments/1j3c2k6/need_help_getting_and_i2c_hid_elan_touchpad_to/)**
  - [x] Single Touch Support
  - [ ] Multi Touch Support
  - [x] Change from freertos polling to GPIO interrupt

