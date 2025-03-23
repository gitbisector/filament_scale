# ESP32-C6 Filament Scale

A smart 3D printer filament scale built with the Seeed XIAO ESP32-C6. Keep track of your filament spools' remaining weight with high precision using the HX711 load cell amplifier.

## Features

- High precision weight measurement using HX711 load cell
- OLED display showing current weight and filament status
- WiFi connectivity with web interface
- Multiple vessel/spool management
- Quick-add vessel feature for easy setup
- Persistent storage of vessels and calibration data
- Real-time weight updates via WebSocket
- Tare and calibration functions

## Hardware Requirements

- Seeed XIAO ESP32-C6 board
- HX711 load cell amplifier
- Load cell (rated for your expected weight range)
- SSD1306 128x64 OLED display (I2C)
- Rotary encoder with push button
- 3D printed case and platform (STL files coming soon)

## Software Requirements

- PlatformIO
- Required libraries (automatically managed by PlatformIO):
  - HX711
  - U8g2lib
  - ESPAsyncWebServer
  - AsyncTCP
  - ArduinoJson

## Pin Configuration

```cpp
// Display I2C
#define I2C_SDA 5
#define I2C_SCL 6

// HX711
#define HX711_DATA_PIN 2
#define HX711_CLOCK_PIN 3

// Rotary Encoder
#define ROTARY_PIN_LEFT 7
#define ROTARY_PIN_RIGHT 8
#define ROTARY_PIN_BUTTON 10
```

## Installation

1. Clone this repository:
   ```bash
   git clone https://github.com/gitbisector/filament_scale.git
   cd filament_scale
   ```

2. Open the project in PlatformIO
3. Build and upload to your ESP32-C6:
   ```bash
   pio run -t upload
   ```

## Usage

### Initial Setup

1. Power on the device
2. Connect to the "Filament Scale" WiFi access point
3. Navigate to http://192.168.4.1 in your browser
4. Configure your WiFi network if desired

### Calibration

1. Place a known weight (e.g., 1kg) on the scale
2. Use the web interface or device menu to start calibration
3. Follow the on-screen instructions

### Adding Vessels

Two ways to add vessels:

1. Quick Add (On Device):
   - Select "Quick Add Vessel" from vessel selection menu
   - Follow the two-step process:
     1. Place empty vessel and confirm
     2. Add 1kg filament spool and confirm

2. Web Interface:
   - Navigate to the web interface
   - Click "Add Vessel"
   - Enter vessel name and weights

### Monitoring Filament

1. Select your vessel from the menu
2. The display will show:
   - Total weight
   - Remaining filament weight
   - Vessel name

## Web Interface

Access the web interface by navigating to the device's IP address. Features include:

- Real-time weight monitoring
- Vessel management (add/edit/delete)
- Calibration controls
- Tare function
- System status

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is open source and available under the [MIT License](LICENSE).

## Acknowledgments

- Thanks to the PlatformIO team for their excellent development platform
- U8g2 library developers for the display driver
- ESPAsyncWebServer team for the web server implementation
