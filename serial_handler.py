import json
import serial
from logs import logging


class SerialHandler:
    def __init__(self):
        try:
            self.serial_port = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)  # Adjust port as needed
        except serial.SerialException as e:
            logging.error(f"Failed to open serial port: {e}")
            raise

    def read_serial_data(self):
        while True:
            try:
                if self.serial_port.in_waiting:
                    serial_data = self.serial_port.readline().decode('utf-8').strip()
                    return json.loads(serial_data)
            except serial.SerialException as e:
                logging.error(f"Serial port error: {e}")
                raise
            except json.JSONDecodeError as e:
                logging.warning(f"Invalid JSON data received: {e}")
                return None

    def send_serial_data(self, data):
        try:
            self.serial_port.write(json.dumps(data).encode('utf-8'))
            print(data)
        except serial.SerialException as e:
            logging.error(f"Failed to send data through serial port: {e}")
