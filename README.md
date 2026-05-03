# HarixOS v1.0

HarixOS is a **real, CLI-based operating system** (not a simulator) for microcontrollers, designed to run directly on hardware via a serial console. It provides a robust kernel with Linux-like commands, a structured API layer for safe hardware control, and a native script engine.

## Supported Hardware

Currently, HarixOS officially supports the **ESP8266** family. However, the architecture is designed to be cross-platform, and support for more microcontrollers (such as ESP32 and others) is planned for future updates.

| Platform | Status | Supported Boards |
|----------|--------|------------------|
| **ESP8266** | ✅ Active | Generic, ESP-01 (1MB), NodeMCU, D1 Mini |
| **ESP32** | 🚧 Planned | ESP32-WROOM, ESP32-C3/S3 |
| **Others** | 🔍 Researching | RP2040, STM32 |

## Screenshots

<p align="center">
  <img src="https://raw.githubusercontent.com/Haris16-code/HarixOS/refs/heads/main/Documentation/screenshots/harixos-screenshot-1.PNG" alt="HarixOS Shell" />
  <br>
  <em>HarixOS Command Shell and Info Command</em>
</p>

## Features

- **Serial command shell** with prompt and help
- **Linux-like filesystem** commands (`pwd`, `cd`, `ls`, `mkdir`, `touch`, `rm`, `cp`, `mv`, `cat`, `write`, `append`)
- **LittleFS-backed storage** with directory support
- **Interactive apps**: Notepad (text editor), Settings (persistent preferences)
- **GPIO API** for safe ESP8266 control (`gpio read|write|mode|pulse`), with boot-strap protection
- **WiFi API** for scanning, connecting, and AP mode
- **System API** for chip info, heap monitoring, reboot
- **HTTP file download** via `pull` command (HTTP/HTTPS)
- **`.hx` script engine** for external app development and automation
- **Lightweight calculator**: `calc <expression>`

## API Layer & .hx Scripts

HarixOS uses a **controlled API layer** that apps interact with instead of raw hardware:

```
┌─────────────────────────┐
│   CLI / .hx Scripts     │  ← User interface
└──────────┬──────────────┘
           │
┌──────────▼──────────────┐
│    HarixOS API Layer    │  ← Structured, safe access
├────────────────────────┤
│ GPIO API               │
│ WiFi API              │
│ System API            │
│ File System API       │
└──────────┬──────────────┘
           │
┌──────────▼──────────────┐
│   ESP8266 Hardware      │  ← Physical hardware
└─────────────────────────┘
```

### .hx Script Engine

HarixOS includes a **powerful script engine** for external app development. Create and run `.hx` automation scripts with:

**Simple Sequential Script:**
```bash
# Example: LED control
print Starting LED test
gpio 2 mode output
gpio 2 on
delay 500
print GPIO2 is HIGH
gpio 2 off
delay 500
print LED test complete
```

**Complex Multi-Step Application:**
```bash
# Example: Device initialization
print === Boot Sequence ===
print Checking system...
system info
system heap
print Initializing GPIO...
gpio 2 mode output
gpio list
print Testing GPIO2...
gpio 2 on
delay 100
gpio 2 off
print All tests passed!
```

**Interactive App Installation:**
```bash
HarixOS> run install myapp
Enter app content (type 'END' on a new line to finish):
print Hello from my app!
gpio 2 on
delay 1000
gpio 2 off
END

HarixOS> run myapp
HarixOS> run list
HarixOS> run uninstall myapp
```


See [Advanced Applications Guide](Documentation/Advanced-Apps.md) for complex app examples and [Development Roadmap](Documentation/ROADMAP.md) for planned enhancements.

## Getting started
1. Build and upload with PlatformIO (default target: generic ESP8266, `esp12e`):

```bash
pio run
pio run --target upload
```

Optional board environments:
- `esp01_1m` (default)
- `esp8266_generic` 
- `nodemcuv2`
- `d1_mini`

Example:

```bash
pio run -e nodemcuv2 --target upload
```

2. Open the serial monitor (115200):

```bash
pio device monitor -b 115200
```

3. Use the prompt `HarixOS>` and type `help` to list commands.

## Documentation

- Full usage guide and deep command docs: [Documentation/README.md](Documentation/README.md)

### Quick Links

**API & Apps:**
- **API Reference**: [Documentation/API.md](Documentation/API.md) — Building apps with HarixOS API
- **Advanced Applications**: [Documentation/Advanced-Apps.md](Documentation/Advanced-Apps.md) — Complex app development guide
- **API Robustness**: [Documentation/API-Robustness.md](Documentation/API-Robustness.md) — Safety and error handling
- **Development Roadmap**: [Documentation/ROADMAP.md](Documentation/ROADMAP.md) — Future features and planned enhancements

**User Guides:**
- **Getting Started**: [Documentation/Getting-Started.md](Documentation/Getting-Started.md)
- **Commands Reference**: [Documentation/Commands.md](Documentation/Commands.md)
- **Script Reference**: [SCRIPT-REFERENCE.md](SCRIPT-REFERENCE.md) — Quick .hx command reference
- **Testing Guide**: [TESTING.md](TESTING.md) — How to test the `run` command and .hx apps
- **Filesystem Guide**: [Documentation/Filesystem.md](Documentation/Filesystem.md)
- **Notepad Guide**: [Documentation/Notepad.md](Documentation/Notepad.md)
- **Apps Guide**: [Documentation/Apps.md](Documentation/Apps.md)
- **Troubleshooting**: [Documentation/Troubleshooting.md](Documentation/Troubleshooting.md)

## Contributing

- Add small apps under `src/apps/<appname>/` and shared helpers under `src/utils/`.
- Extend the API layer in `src/api/` for new hardware features.
- Keep apps safe: always use API functions instead of raw hardware access.

## Community

### Join HarixOS Community For Discussion
Got questions, ideas, or cool projects built with HarixOS? Join our community!
- **Discussions**: [HarixOS Community]()
