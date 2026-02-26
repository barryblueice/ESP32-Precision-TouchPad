import sys
from PySide6.QtWidgets import QApplication, QMainWindow, QPushButton, QVBoxLayout, QWidget, QStatusBar
from usb_module import USBCommunicatorThread

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("ESP32 PTP DFU")
        
        self.setStatusBar(QStatusBar(self))
        self.statusBar().showMessage("Initializing...")

        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        layout = QVBoxLayout(central_widget)
        layout.setContentsMargins(10, 10, 10, 10)

        self.btn_action = QPushButton("Waiting for connect...")
        self.btn_action.setFixedSize(300, 50)
        self.btn_action.setEnabled(False)
        self.btn_action.clicked.connect(self.on_button_clicked)
        layout.addWidget(self.btn_action)

        self.usb_thread = USBCommunicatorThread()
        
        self.usb_thread.device_event.connect(self.handle_device_event)
        
        self.usb_thread.status_message.connect(self.statusBar().showMessage)
        
        self.usb_thread.start()

        self.adjustSize() 
        self.setFixedSize(self.sizeHint())

    def handle_device_event(self, is_connected):
        if is_connected:
            self.btn_action.setEnabled(True)
            self.btn_action.setText("Enter DFU Mode")
        else:
            self.btn_action.setEnabled(False)
            self.btn_action.setText("Waiting for connect...")

    def on_button_clicked(self):
        self.statusBar().showMessage("Sending DFU command...")
        self.usb_thread.send_packet()

    def closeEvent(self, event):
        self.usb_thread.stop()
        event.accept()

if __name__ == "__main__":
    app = QApplication(sys.argv)

    window = MainWindow()
    window.show()
    sys.exit(app.exec())