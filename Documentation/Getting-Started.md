Getting Started with HarixOS
============================

Prerequisites
- ESP8266 Flasher Tool if using precompiled
- PlatformIO if you use build at your own (installed in VS Code or via pip)
- USB-to-serial adapter or development board for ESP8266
- Any ESP8266-family board (ESP-01, ESP-12E, NodeMCU, D1 mini, etc.)

## Precompiled Binaries

If you don’t want to build HarixOS manually, you can use precompiled firmware files.

## Download binaries

Prebuilt Latest `.bin` files are available on the [release page](https://github.com/Haris16-code/HarixOS/releases).

### Example (ESP8266 NodeMCU):


### Flash Binary:

### [HarixOS Web Flasher (Recommended)](https://harixos.harislab.tech/harixos-web-flasher/)

**or use any other ESP8266 Flasher**

## Flash addresses (ESP8266)
- `0x00000` → `firmware.bin`

> ⚠️ Make sure the binary matches your board (`esp01` / `nodemcu` / `d1 mini`/ `Generic ESP8266`).

Serial console:
### [HarixOS Web Serial Console (Recommended)](https://harixos.harislab.tech/web-serial-monitor/)

---

## If You Want To Build and flash On Your Own
1. Open the project in VS Code with the PlatformIO extension (or use the CLI).
2. Build:

```bash
pio run
```

3. Upload (ensure correct COM port; some boards may need boot/program mode wiring):

```bash
pio run --target upload
```

Board environments 
 - `esp01_1m` (default)
- `esp8266_generic`
- `nodemcuv2`
- `d1_mini`

Example (NodeMCU):

```bash
pio run -e nodemcuv2 --target upload
```

Serial console
1. Start the monitor at 115200 baud:

**Putty** Is Recommended

---

2. You should see the HarixOS banner (unless `settings` disables it). Use `HarixOS>` prompt.

Common commands
- `help` — list available commands
- `info` — system and Wi-Fi summary
- `pwd` / `ls` — check filesystem
- `notepad /myfile.txt` — try editing and saving a text file
- `gpio list` — show available GPIO pins
- `wifi scan` — scan nearby networks
- `run /scripts/myapp.hx` — execute a .hx script

See Full Commands Documentation: [View Here](Commands.md)
## .hx Scripts (Automation & External Apps)

HarixOS supports `.hx` script files for automation and external app development. Scripts use a simple line-based API:

```bash
# Create a blink script
notepad /scripts/blink.hx

# Add these lines:
gpio 2 mode output
gpio 2 on
delay 500
gpio 2 off
delay 500

# Save and run:
# HarixOS> run /scripts/blink.hx
```

**Common script commands:**
- `gpio <pin> on|off|read|toggle` — control GPIO
- `delay <ms>` — wait (milliseconds)
- `wifi scan|connect <ssid> <pass>|status` — WiFi control
- `system info|reboot` — system commands

See [API Documentation](./API.md) for complete scripting reference.

## Using the API Layer

HarixOS provides a **structured API layer** for building safe applications. Apps never access hardware directly—they always go through the API:

**For CLI developers:** Commands use the API internally.

**For external app developers:** Create .hx scripts or C++ apps that call API functions like:
```cpp
#include "api/gpio_api.h"
harixos::api::GpioAPI::write(2, HIGH);
```

This design ensures:
- ✅ Apps can't crash the system
- ✅ Boot-critical pins are protected
- ✅ All hardware access is logged and validated

See [API Documentation](./API.md) for complete reference.

## Notes about hardware
- Usable GPIO pins differ by board. HarixOS blocks ESP8266 flash pins (GPIO6-11) and serial pins (GPIO1/3) by default for safety.
- Boot pins (GPIO0/GPIO2/GPIO15) should not be forced to invalid levels during reset.
- `adc` reports raw A0 input; available range and scaling depend on your board.

Upgrading
- Make changes and re-run `pio run --target upload`.
- Files on LittleFS persist across uploads only if flashing does not erase the filesystem; use `fs format` to reset storage.
