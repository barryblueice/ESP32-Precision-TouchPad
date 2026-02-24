import usb.core
import usb.backend.libusb1
from PySide6.QtCore import QThread, Signal
import hid
import os

VID = 0x0D00
PID = 0x072A
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
    
    def send_packet(self):
        packet = bytes(0xFF for _ in range(REPORT_SIZE))
        self.dev.write(packet)

    def run(self):
        self.status_message.emit("USB Monitor started...")
        while self.running:
            try:
                dev_found = usb.core.find(idVendor=VID, idProduct=PID, backend=backend)
                
                is_hid = False
                if dev_found:
                    for cfg in dev_found:
                        for intf in cfg:
                            if intf.bInterfaceClass == 0x03:
                                is_hid = True
                                break
                
                if is_hid and not self.device_present:
                    try:
                        self.dev.open(VID, PID)
                        self.device_present = True
                        self.device_event.emit(True)
                        self.status_message.emit(f"Device connected: (VID:{hex(VID)} PID:{hex(PID)})")
                    except Exception as e:
                        self.status_message.emit(f"Open Failed: {str(e)}")

                elif not is_hid and self.device_present:
                    self.device_present = False
                    self.dev.close()
                    self.device_event.emit(False)
                    self.status_message.emit("Device disconnected")
                    
            except Exception as e:
                pass

            self.msleep(500)

    def stop(self):
        self.running = False
        self.dev.close()
        self.wait()