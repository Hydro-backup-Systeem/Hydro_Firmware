import serial
import struct
import time
from rich.console import Console
from rich.progress import BarColumn, Progress

# Serial port configuration (change to match your system)
SERIAL_PORT = "/dev/ttyACM0"  # Change to your serial port (e.g., "/dev/ttyUSB0" for Linux/Mac)
BAUD_RATE = 115200

# ADC settings
ADC_MAX = 4095  # 12-bit resolution
BAR_WIDTH = 50   # Width of the gain bar

console = Console()

def read_serial_data(ser):
  """Reads 2-byte data from the serial port and converts it to a 12-bit value."""
  data = ser.read(2)  # Read 2 bytes
  if len(data) == 2:
    value = struct.unpack("<H", data)[0]  # Little-endian 16-bit integer
    return value & 0x0FFF  # Mask to 12-bit value
  return None

def display_gain_bar(value):
  """Displays a dynamic gain bar."""
  scaled_value = int((value / ADC_MAX) * BAR_WIDTH)
  bar = "█" * scaled_value + " " * (BAR_WIDTH - scaled_value)
  console.print(f"[{bar}] {value}/{ADC_MAX}", end="\r")

def main():
  """Main function to read from serial and display the gain bar."""
  with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1) as ser:
    console.print("[cyan]Microphone Gain Adjuster Running...[/cyan]")
    time.sleep(1)

    while True:
      value = read_serial_data(ser)
      if value is not None:
        display_gain_bar(value)

if __name__ == "__main__":
  main()
