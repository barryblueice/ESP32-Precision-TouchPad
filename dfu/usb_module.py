import usb.core
import usb.backend.libusb1
from PySide6.QtCore import QThread, Signal
import hid
import os

TARGET_DEVICES = [
    (0x0D00, 0x072A),
    (0x0D00, 0x072B),
    (0x0D00, 0x072C),
    (0x0D00, 0x072D),
]

REPORT_SIZE = 64

current_dir = os.path.dirname(os.path.abspath(__file__))
lib_path = os.path.join(current_dir, 'libusb-1.0.dll')
backend = usb.backend.libusb1.get_backend(find_library=lambda x: lib_path)

class USBCommunicatorThread(QThread):
    device_event = Signal(bool)
    status_message = Signal(str)

    def __init__(self):
        super().__init__()
        self.running = True
        self.device_present = False
        self.dev = hid.device()
        self.target_interface = 0 

    def send_packet(self):
        packet = [0xFF] + [0xFF] * (REPORT_SIZE)
        if self.device_present:
            try:
                self.dev.write(packet)
            except Exception as e:
                self.status_message.emit(f"Write error: {e}")

    def run(self):
        self.status_message.emit("USB Monitor started...")
        while self.running:
            try:
                device_info = None
                for vid, pid in TARGET_DEVICES:
                    VID = vid
                    PID = pid
                    for d in hid.enumerate(vid, pid):
                        if d['interface_number'] == self.target_interface:
                            device_info = d
                            break
                    if device_info:
                        break

                if device_info and not self.device_present:
                    try:
                        self.dev.open_path(device_info['path'])
                        self.dev.set_nonblocking(True)
                        self.device_present = True
                        self.device_event.emit(True)
                        self.status_message.emit(f"Device connected: (VID:{hex(VID)} PID:{hex(PID)})")
                    except Exception as e:
                        self.status_message.emit(f"Open Failed: {str(e)}")

                elif not device_info and self.device_present:
                    self.device_present = False
                    self.dev.close()
                    self.device_event.emit(False)
                    self.status_message.emit("Device disconnected")

            except Exception as e:
                pass

            self.msleep(500)

    def stop(self):
        self.running = False
        if self.device_present:
            self.dev.close()
        self.wait()