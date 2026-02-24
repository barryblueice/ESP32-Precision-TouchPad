import sys
from PySide6.QtWidgets import QApplication, QMainWindow, QPushButton, QVBoxLayout, QWidget, QStatusBar

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("ESP32 PTP DFU")

        self.setStatusBar(QStatusBar(self))
        self.statusBar().showMessage("No ESP32 PTP device found")

        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        layout = QVBoxLayout(central_widget)
        layout.setContentsMargins(10, 10, 10, 10) 

        self.button = QPushButton("Enter DFU")
        
        self.button.setFixedSize(300, 50)
        
        self.button.clicked.connect(self.on_button_clicked)
        layout.addWidget(self.button)

        self.adjustSize()
        
        self.setFixedSize(self.sizeHint())

    def on_button_clicked(self):
        self.statusBar().showMessage("Entering DFU mode...", 2000)

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec())