[英文](https://github.com/barryblueice/ESP32-Precision-TouchPad/blob/main/README.md) | [简体中文](https://github.com/barryblueice/ESP32-Precision-TouchPad/blob/main/README_CN.md)

<img width="900" height="723" alt="image" src="https://github.com/user-attachments/assets/6dd84c6f-8089-4962-9cd4-7cbeaad9e661" />

# **这是什么？**

一个基于ESP32-S2 + 义隆ELAN触摸板（来自联想库存备件）的USB外设设备，支持Microsoft精确式触摸板标准，支持Windows触摸手势。
同时包含了一个HP指纹模块，用于Windows Hello指纹登录。

### **硬件开源地址：[基于ESP32的Windows精确式触摸板](https://oshwhub.com/barryblueice/esp32-precision-touchpad)**

#### **精确式触摸板模式演示：**

https://github.com/user-attachments/assets/256ca4d0-b585-47dc-b8cc-44001de3028a

#### **鼠标兼容模式演示：**

https://github.com/user-attachments/assets/cb5ae0a1-41e6-4b15-b257-127867edca49

#### **安卓系统演示**:

https://github.com/user-attachments/assets/dce2b15a-eb64-4c0a-b503-1ea32e15b02b

#### **应用:**

 - **触摸板主PCB:**

<img width="900" height="667" alt="image" src="https://github.com/user-attachments/assets/a25d1b9c-eece-432a-9ca9-677dc87e56ad" />

<img width="900" height="623" alt="image" src="https://github.com/user-attachments/assets/813ec446-ebc3-422f-ba02-ab4217bdd4ef" />

 - **2.4G接收器:**

<img width="900" height="517" alt="image" src="https://github.com/user-attachments/assets/2af38202-edb0-4c51-ac50-329084c07bd0" />

<img width="900" height="504" alt="image" src="https://github.com/user-attachments/assets/e1ca4a4e-5156-40c2-ac76-7019ce21d6cf" />

 - **外观设计 (SOLIDWORKS建模):**

<img width="900" height="506" alt="image" src="https://github.com/user-attachments/assets/42be22d2-275e-4799-bdfd-6f022412048f" />

<img width="900" height="506" alt="image" src="https://github.com/user-attachments/assets/bb75f841-200c-4c47-9cbe-ed900720e240" />

<img width="900" height="506" alt="image" src="https://github.com/user-attachments/assets/28aea2f9-f43a-4217-a47e-e8b8bca49d43" />

 - **系统应用::**

<img width="501" height="500" alt="image" src="https://github.com/user-attachments/assets/ca5a84f5-3594-45f0-bfe8-55217592828e" />

<img width="536" height="182" alt="image" src="https://github.com/user-attachments/assets/9c88e2ca-030a-46b5-9981-c7615a9c184d" />

***

# TODO List:
  ## Hardware:
  - [x] PCB设计
  - [x] 材料选型
  - [x] 外壳设计 (SOLIDWORKS建模)
  - [x] 为后续的2.4G无线支持添加电池（2.4G无线模式下指纹模块不可用）
  - [x] 为后续的蓝牙支持添加电池（蓝牙无线模式下指纹模块不可用）
  - [ ] ~~添加ThinkPad TrackPoint小红点~~（该功能目前暂无计划更新）
  - [ ] ~~添加特殊定义微动按钮~~（该功能目前暂无计划更新）

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
  - [x] 添加新的HID端口以支持Mouse Mode兼容<br>
        （对于部分不支持PTP的老系统/BIOS可用，例如Windows 7）
  - [ ] 2.4G无线模式开发
  - [ ] 蓝牙无线模式开发

***

# 目前仍然存在的问题：

 - 双指轻触（相当于鼠标右键单击）只能在笔记本平台上触发。
  > 关于台式机触发双指轻触的解决方案，参见[系统调优指南](https://github.com/barryblueice/ESP32-Precision-TouchPad/wiki/%E7%B3%BB%E7%BB%9F%E8%B0%83%E4%BC%98%E6%8C%87%E5%8D%97)
 - 为兼容而设计的HID多端口方案在部分老系统上可能无法使用，例如Windows XP。
 - 不能快速连续单击。
 - 由于标准限制，原生PTP Mode仅支持有线连接模式/2.4G无线模式。<br>蓝牙模式需要在ESP32端处理手势，并且只能支持一小部分自定义手势。

 ***

# 目前支持的系统（已经过测试）：

 - Windows 7 （鼠标模式），Windows 10/11（精确式触摸板模式）；
 - Ubuntu 22.04及更新版本系统；
 - Color OS 17 （基于Android 16），测试设备为一加Ace2（精确式触摸板模式）；
 - HP / MSI BIOS（鼠标模式）

***

# 关于方案实现：

可前往[wiki page](https://github.com/barryblueice/ESP32-Precision-TouchPad/wiki)了解更多详细内容。

***

# ~~相关的衍生计划：~~

 - ~~仍在开发计划中~~
