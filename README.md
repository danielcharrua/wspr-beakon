# WSPR Beakon

A multi-band WSPR (Weak Signal Propagation Reporter) beacon transmitter based on ESP32 microcontroller with Si5351 clock generator, GPS synchronization, WiFi connectivity, and band-switching capabilities.

## Features

- **Multi-band transmission**: 2200m to 2m amateur radio bands
- **Automatic time synchronization**: Via GPS or NTP over WiFi
- **Band switching**: Automated filter relay control
- **LCD display**: Real-time status and transmission countdown
- **Rotary encoder**: Easy frequency selection
- **GPS support**: Precise time synchronization with NEO-7 module
- **WiFi connectivity**: Multiple network support for NTP sync
- **Power control**: Configurable Si5351 drive levels

## Hardware Requirements

- ESP32 DevKit C V4 (or compatible)
- Si5351 clock generator module
- GPS module (NEO-7 or compatible) - Optional
- 16x2 LCD with I2C interface
- Rotary encoder with push button
- UDN2981 relay driver IC (for band switching)
- Relay modules for antenna switching
- Low-pass filters for each band

For detailed hardware assembly instructions, refer to the `helpers/Instrucciones_de_montaje.pdf` file.

## Software Installation

### Prerequisites

1. **Arduino IDE**: Download and install from [arduino.cc](https://www.arduino.cc/en/software)
2. **ESP32 Board Package**: Install ESP32 support in Arduino IDE

### Arduino IDE Configuration

Configure Arduino IDE with the following settings (see `helpers/Arduino_ESP32_DevKit_C_V4_config.png`):

- **Board**: "ESP32 Dev Module"
- **Upload Speed**: 921600
- **CPU Frequency**: 240MHz (WiFi/BT)
- **Flash Frequency**: 80MHz
- **Flash Mode**: QIO
- **Flash Size**: 4MB (32Mb)
- **Partition Scheme**: Default 4MB with spiffs
- **Core Debug Level**: None
- **PSRAM**: Disabled

### Library Installation

1. Extract all ZIP files from the `lib/` folder
2. Copy the extracted folders to your Arduino libraries directory:
   - **Windows**: `Documents\Arduino\libraries\`
   - **macOS**: `~/Documents/Arduino/libraries/`
   - **Linux**: `~/Arduino/libraries/`

Required libraries:

- Etherkit Si5351 (v2.1.4)
- JTEncode (v1.2.0)
- LiquidCrystal I2C (v1.1.4)
- NTPClient (v3.1.0)
- RotaryEncoder (v1.5.2)
- Time (v1.5)
- TinyGPSPlus (v1.1.0)

### Firmware Upload

1. Connect your ESP32 to your computer via USB cable
2. Open `wspr-beakon.ino` in Arduino IDE
3. Configure the required parameters (see Configuration section below)
4. Select the correct COM port in Arduino IDE
5. Click "Upload" to flash the firmware

## Configuration

Before uploading the firmware, you must modify the configuration variables in the `USER CONFIGURATION` section of the code:

### Basic WSPR Configuration

```cpp
// WSPR transmitter configuration
#define WSPR_CALL "EA1REX"  // Your 4-6 character callsign
#define WSPR_LOC "IN53"     // Your 4-character grid locator
#define WSPR_DBM 20         // Power level shown in WSPR frame (0, 3, 7, 10 dBm)
#define WSPR_TX_EVERY 4     // Transmit every X minutes (2, 4, 6, 8, 10, etc.)
```

### Si5351 Power Level

```cpp
// Available options: SI5351_DRIVE_2MA, SI5351_DRIVE_4MA, SI5351_DRIVE_6MA, SI5351_DRIVE_8MA
// Power output: 2MA = 0.6mW, 4MA = 2.5mW, 6MA = 5mW, 8MA = 10mW
#define SI5351_DRIVE_LEVEL SI5351_DRIVE_4MA
```

### WiFi Networks

Add your WiFi networks for NTP synchronization:

```cpp
const WiFiNetwork wifiNetworks[] = {
  {"Your_SSID_1", "password1"},
  {"Your_SSID_2", "password2"},
  {"Your_SSID_3", "password3"}
};
```

### Band Configuration

Configure frequencies, crystal calibration, and relay pins for each band:

```cpp
} wsprFrequencies[] = {
  // Frequency(Hz), Crystal_Freq(Hz), Label, Relay_Pin
  {144489000UL, 25000000UL, "144.489 MHz 2m",  32},   // 2m band
  {70105048UL,  25000000UL, "70.091 MHz 4m",   32},   // 4m band
  {50303500UL,  25000000UL, "50.293 MHz 6m",   32},   // 6m band
  {28131120UL,  25000000UL, "28.124 MHz 10m",  33},   // 10m band
  {24924600UL,  25000000UL, "24.924 MHz 12m",  33},   // 12m band
  {21099330UL,  25000000UL, "21.094 MHz 15m",  25},   // 15m band
  {18104600UL,  25000000UL, "18.104 MHz 17m",  25},   // 17m band
  {14099615UL,  25000000UL, "14.095 MHz 20m",  26},   // 20m band
  {10142033UL,  25000000UL, "10.138 MHz 30m",  27},   // 30m band
  {7041356UL,   25000000UL, "7.038 MHz 40m",   27},   // 40m band
  {5287200UL,   25000000UL, "5.287 MHz 60m",   14},   // 60m band
  {3570732UL,   25000000UL, "3.568 MHz 80m",   14},   // 80m band
  {1838426UL,   25000000UL, "1.836 MHz 160m",  12},   // 160m band
  {475786UL,    25000000UL, "0.474 MHz 630m",  32},   // 630m band
  {136000UL,    25000000UL, "0.136 MHz 2200m", 32}    // 2200m band
};
```

### Relay Pin Assignment

The following ESP32 pins are used for band relay control:

- Pin 12: 160m, 630m bands
- Pin 14: 60m, 80m bands
- Pin 25: 15m, 17m bands
- Pin 26: 20m band
- Pin 27: 30m, 40m bands
- Pin 32: 2200m, 630m, 4m, 2m bands
- Pin 33: 10m, 12m bands

**Important Notes:**

1. **Initial Crystal Frequency**: For the first upload, set all crystal frequencies to `25000000UL` (25 MHz)
2. **Calibration Required**: After initial upload, you'll need to calibrate each band for frequency accuracy
3. **Custom Frequencies**: The provided frequencies are examples - you must calculate your own based on your specific Si5351 crystal

## Post-Installation Calibration

After uploading the firmware with default settings:

1. **Frequency Calibration**: Each band requires individual crystal frequency calibration for accuracy
2. **Power Measurement**: Verify actual RF output power matches your requirements
3. **Filter Verification**: Ensure band-switching relays activate the correct low-pass filters
4. **GPS Testing**: Verify GPS module acquires satellites and provides accurate time
5. **WiFi Testing**: Confirm NTP synchronization works with your networks

⚠️ **Important**: This completes the software installation. For frequency calibration and fine-tuning procedures, refer to the hardware assembly manual.

## Legal Notice

This code is provided "as is," without any warranties, express or implied. The user assumes full responsibility for its use, implementation, and any consequences.

**Important:**

- Ensure that WSPR usage complies with local radio frequency regulations
- The author is not responsible for equipment damage, interference, or legal violations
- This code is for educational and experimental purposes only

By using this code, you acknowledge that you do so at your own risk.

## License

See `LICENSE.txt` for license information.
