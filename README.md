# WSPR Beakon

A modular and expandable WSPR (Weak Signal Propagation Reporter) beacon transmitter based on ESP32 microcontroller with Si5351 clock generator. Designed with a modular approach that allows from basic configurations to advanced systems with multiple enhancements.

![WSPR Beakon featured image](./assets/wspr-beakon-pcb.webp)

## Main Features

- **Multiband transmission**: From 2200m to 2m (hardware limited to 10m)
- **Built-in modular filter socket**: Easy monoband filter installation and swapping
- **Automatic synchronization**: Via GPS or NTP over WiFi
- **LCD display**: Real-time status and transmission countdown
- **Rotary encoder**: Easy frequency selection
- **Power control**: Configurable Si5351 drive levels
- **WiFi connectivity**: Multiple network support

## Documentation

All usage instructions, assembly guides, configuration, and technical details are available in the official documentation site:

- [🇬🇧 English Documentation](https://danielcharrua.github.io/wspr-beakon-docs/docs/intro)
- [🇪🇸 Documentación en Español](https://danielcharrua.github.io/wspr-beakon-docs/es/docs/intro)

## Credits

- **EB1A (Javier)** - RF engineering, hardware design, PCB development, and project photography
- **EA1REX (Daniel)** - Software development, firmware implementation, hardware design, and PCB development

## License

- Software: [MIT License](LICENSE.txt)
- Hardware: [CERN-OHL-S License](LICENSE-HARDWARE.txt)
