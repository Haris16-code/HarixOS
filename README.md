# HarixOS
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
  <em>HarixOS Shell</em>
</p>
<p align="center">
  <img src="https://github.com/Haris16-code/HarixOS/blob/main/Documentation/screenshots/harixos-info.PNG?raw=true" alt="HarixOS Info" />
  <br>
</p>

## Features
- **CLI-based operating system shell** with interactive command prompt and help system  
- **System information tools** (`info`, `chip`, `heap`, `uptime`) for monitoring ESP8266 status  
- **Power and control commands** (`reboot`, `adc`) for direct hardware interaction  
- **Time management system** with time display, sync support, and scheduling capabilities  
- **Background task scheduler** for running commands and scripts at specific times  
- **Online update system** with update checking and version management (`update check`)  
- **HTTP file downloader** via `pull <url> <path>` for fetching files from the internet  

---

- **Linux-like filesystem support** using LittleFS  
  (`pwd`, `cd`, `ls`, `mkdir`, `touch`, `rm`, `cp`, `mv`, `cat`, `write`, `append`)  
- **Directory-based file operations** with recursive listing support (`ls -R`)  
- **Filesystem utilities layer** (`fs ...`) for advanced storage management  

---

- **Hardware control system** with GPIO management (`gpio ...`) including safe pin handling  
- **WiFi management system** for scanning, connecting, and network configuration (`wifi ...`)  
- **I2C bus tools** for peripheral communication and sensor integration  

---

- **Application system** with runtime app handling (`run ...`) for install, execute, and manage `.hx` apps  
- **Interactive text editor (Notepad)** for file editing directly from CLI  
- **System settings manager** for persistent configuration control  
- **Built-in calculator** for arithmetic expression evaluation (`calc <expr>`)  
- **HTTP server tools** for file sharing and simple hosting (`serve ...`)  

---

- **Help system with categorized topics** (`wifi`, `gpio`, `fs`, `serve`, `time`, `schedule`, `update`)  
- **Script engine support (.hx files)** for automation and external app development  
- **Modular API architecture** for hardware, system, and networking layers  
- **Extensible command system** designed for future boards (ESP32, etc.) 

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

---
## [Getting Started](Documentation/Getting-Started.md)
---

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
- **Discussions**: [HarixOS Community](https://harixos.harislab.tech)
