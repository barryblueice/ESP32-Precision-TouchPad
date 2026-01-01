[英文](https://github.com/barryblueice/ESP32-Precision-TouchPad/blob/main/README.md) | [简体中文](https://github.com/barryblueice/ESP32-Precision-TouchPad/blob/main/README_CN.md)

# **这是什么？**

一个基于ESP32-S2 + 义隆ELAN触摸板（来自联想库存备件）的USB外设设备，支持Microsoft精确式触摸板标准，支持Windows触摸手势。
同时包含了一个HP指纹模块，用于Windows Hello指纹登录。

### **硬件开源地址：[基于ESP32的Windows精确式触摸板](https://oshwhub.com/barryblueice/esp32-precision-touchpad)**

https://github.com/user-attachments/assets/256ca4d0-b585-47dc-b8cc-44001de3028a

<img width="1194" height="970" alt="image" src="https://github.com/user-attachments/assets/35bba23a-5c70-4f4c-b9e3-434b141dd59f" />

<img width="826" height="823" alt="image" src="https://github.com/user-attachments/assets/ca5a84f5-3594-45f0-bfe8-55217592828e" />

<img width="536" height="182" alt="image" src="https://github.com/user-attachments/assets/9c88e2ca-030a-46b5-9981-c7615a9c184d" />

***

# TODO List:
  ## Hardware:
  - [x] PCB设计
  - [x] 材料选型
  - [ ] 外壳设计 (SOLIDWORKS建模)
  - [ ] 为后续的蓝牙支持添加电池（蓝牙无线模式下指纹模块不可用）
  - [ ] 添加ThinkPad TrackPoint小红点
  - [ ] 添加特殊定义微动按钮

  ## Software:
  - [x] Microsoft精确式触摸板握手
  - [x] ELAN TouchPad驱动
  - [x] **从Mouse Mode (仅支持单指) 切换到Absolute Mode (支持多指), [感谢@ApprehensiveAnt9858](https://www.reddit.com/r/embedded/comments/1j3c2k6/need_help_getting_and_i2c_hid_elan_touchpad_to/)**
  - [x] 单指触摸支持
  - [x] 多指触摸支持
    - [x] 多指滑动手势支持
    - [x] 单指Tap支持
    - [x] 多指Tap支持（小概率可能触发不成功）
  - [x] 物理按键支持（左键 & 右键）
  - [x] 由freertos轮询切换到GPIO Int中断触发
  - [ ] 在鼠标模式和精确式触摸板模式中自动切换

***

# 关于方案实现：

可前往[wiki page](https://github.com/barryblueice/ESP32-Precision-TouchPad/wiki)了解更多详细内容。
