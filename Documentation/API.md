# HarixOS API Documentation

## Overview

HarixOS provides a **complete API layer** designed specifically for **safe execution of .hx scripts and external applications**. Instead of writing bare hardware code, `.hx` apps and scripts interact through this well-defined API.

The API is built with robustness and safety as core principles:
- **Boot Safety**: Prevents unsafe GPIO states during boot
- **Resource Protection**: Validates all operations before hardware access
- **Error Handling**: All operations return structured results with status codes
- **Extensibility**: New commands can be easily added to support new functionality

```
Hardware (ESP8266 registers)
         ↓
    Arduino Framework
         ↓
    HarixOS API Layer (.hx Script Engine)
         ↓
    .hx Scripts / External Apps / CLI Commands
```

---

## Architecture

### Why an API Layer?

1. **Safety**: Prevent apps from crashing the entire system
2. **Consistency**: All access follows the same patterns
3. **Control**: OS can enforce resource limits, boot safety, etc.
4. **Abstraction**: Apps don't need to know hardware details

### API Modules

```
src/api/
  ├── api_types.h        # Core types and result structures
  ├── gpio_api.h/cpp     # GPIO control
  ├── wifi_api.h/cpp     # WiFi networking
  ├── system_api.h/cpp   # System information and reboot
  ├── script_engine.h/cpp # .hx script interpreter
```

---

## GPIO API

### Safe GPIO Operations

The GPIO API prevents dangerous states (e.g., GPIO0 going LOW on ESP-01, which blocks boot):

```cpp
#include "api/gpio_api.h"
using namespace harixos::api;

// Write GPIO (automatically blocks unsafe boot strap pins)
ApiResult res = GpioAPI::write(2, HIGH);
if (res.isSuccess()) {
  Serial.println(res.message);
}

// Read GPIO
GpioResult res = GpioAPI::read(2);
if (res.isSuccess()) {
  Serial.printf("GPIO2 = %d\n", res.value);
}

// Set mode
GpioAPI::setMode(2, OUTPUT);
GpioAPI::setMode(4, INPUT_PULLUP);

// Pulse GPIO
GpioAPI::pulse(2, 5, 250);  // 5 pulses, 250ms each
```

### Available Pins

**ESP-01:**
- GPIO0, GPIO2 (both required HIGH for normal boot)

**ESP8266 (4MB+):**
- GPIO0, GPIO2, GPIO4, GPIO5, GPIO12, GPIO13, GPIO14, GPIO15, GPIO16
- GPIO15 must be LOW at boot
- GPIO6-11 reserved for flash
- GPIO1/3 reserved for UART

---

## WiFi API

```cpp
#include "api/wifi_api.h"
using namespace harixos::api;

// Scan networks
WiFiAPI::scan(Serial);

// Connect
ApiResult res = WiFiAPI::connect("MySSID", "password", 10000);
if (res.isSuccess()) {
  Serial.println("Connected!");
}

// Get status
WiFiAPI::status(Serial);

// Get IP info
WiFiAPI::getIP(Serial);

// Access Point
WiFiAPI::startAP("HarixOS", "12345678", 1);
WiFiAPI::stopAP();
```

---

## System API

```cpp
#include "api/system_api.h"
using namespace harixos::api;

// Get system info
SystemInfo info = SystemAPI::getInfo();
Serial.printf("Chip: 0x%08X, Heap: %u bytes\n", info.chipId, info.heapFree);

// Print formatted info
SystemAPI::printInfo(Serial);

// Utilities
uint32_t freeHeap = SystemAPI::getHeapFree();
uint32_t uptime = SystemAPI::getUptime();
uint32_t chipId = SystemAPI::getChipID();

// Control
SystemAPI::delay(1000);
SystemAPI::reboot();
```

---

## .hx Script Format

HarixOS supports `.hx` script files for automation and external app development. Scripts are the primary way applications interact with hardware through the API.

### Installing and Running Scripts

```bash
# Install a script (interactive - you'll be prompted for content)
run install myapp

# List all installed scripts
run list

# Run an installed script by name
run myapp

# Uninstall a script
run uninstall myapp
```

### Script Syntax

Scripts are line-based commands using the API. Each line is a separate command:

```bash
# Output to Console
print Starting GPIO test...
print
print GPIO2 is now HIGH

# GPIO Control
gpio 2 on              # Set GPIO2 HIGH
gpio 2 off             # Set GPIO2 LOW
gpio 2 read            # Read GPIO2 value
gpio 2 toggle          # Toggle GPIO2
gpio 2 mode output     # Set to output mode
gpio 2 mode input_pullup  # Set to input with pullup
gpio 2 pulse 10 500    # Pulse 10 times, 500ms delay
gpio list              # Show available GPIO pins

# Delays
delay 1000             # Wait 1000ms

# WiFi
wifi scan              # Scan networks
wifi connect MySSID password  # Connect to WiFi
wifi disconnect        # Disconnect
wifi status            # Show WiFi status
wifi ip                # Show IP configuration

# System
system info            # Show system information
system heap            # Show heap free
system reboot          # Reboot the device

# Comments
# This is a comment
help                   # Show available commands
```

### Print Command (Output)

The `print` command outputs text to the serial console:

```bash
print Hello, HarixOS!
print
print This is a blank line above
print GPIO2 value: 1
print Test completed
```

**Notes:**
- `print` with no text prints a blank line
- Text is printed as-is (no formatting required)
- Useful for status messages and debugging

### Example Scripts

**Blink LED (hello.hx):**
```bash
print =====================================
print HarixOS Blink Test
print =====================================
print
print Setting GPIO2 to OUTPUT mode...
gpio 2 mode output
print GPIO2 is ready
print
print Blinking LED 5 times...
gpio 2 pulse 5 250
print
print Blink test complete!
```

**System Info App (sysinfo.hx):**
```bash
print =====================================
print System Information
print =====================================
print
print Gathering system details...
system info
print
print Memory Status:
system heap
print
print Done!
```

**WiFi Test App (wifi_test.hx):**
```bash
print =====================================
print WiFi Test
print =====================================
print
print Current WiFi Status:
wifi status
print
print IP Configuration:
wifi ip
print
print Scanning for networks...
wifi scan
print
print WiFi test complete!
```

**GPIO Control App (gpio_test.hx):**
```bash
print =====================================
print GPIO Control Test
print =====================================
print
print Available GPIO pins on this device:
gpio list
print
print Setting GPIO2 to OUTPUT...
gpio 2 mode output
print GPIO2 mode set
print
print Setting GPIO2 HIGH...
gpio 2 on
print Reading GPIO2...
gpio 2 read
print
print Setting GPIO2 LOW...
gpio 2 off
print Reading GPIO2...
gpio 2 read
print
print GPIO test complete!
```

---

## Return Values (API Results)

All API functions return structured results:

```cpp
struct ApiResult {
  ApiStatus status;      // API_OK, API_ERROR, API_INVALID_PIN, etc.
  String message;        // Human-readable message
  
  bool isSuccess() const;
  bool isError() const;
};
```

### Status Codes

| Code | Meaning |
|------|---------|
| `API_OK` | Success |
| `API_ERROR` | Generic error |
| `API_INVALID_PIN` | GPIO pin doesn't exist or is reserved |
| `API_INVALID_GPIO_MODE` | Invalid GPIO mode |
| `API_FILE_NOT_FOUND` | File doesn't exist |
| `API_WIFI_NOT_CONNECTED` | WiFi not connected |
| `API_WIFI_CONNECTION_FAILED` | WiFi connection attempt failed |
| `API_INVALID_ARGUMENT` | Invalid function argument |
| `API_TIMEOUT` | Operation timed out |
| `API_OUT_OF_MEMORY` | Insufficient memory |
| `API_NOT_IMPLEMENTED` | Feature not yet implemented |

---

## Safety Features

### Boot Strap Protection

GPIO0 and GPIO2 cannot be set LOW on ESP-01 or ESP8266 (required for normal boot):

```bash
gpio 0 off   # ERROR: Cannot set GPIO0/GPIO2 LOW
gpio 2 off   # ERROR: Cannot set GPIO0/GPIO2 LOW
gpio 0 toggle  # ERROR: Cannot toggle boot pins
```

### GPIO Validation

```bash
gpio 1 on    # ERROR: GPIO1 is UART TX (reserved)
gpio 7 on    # ERROR: GPIO6-11 are flash memory (reserved)
gpio 99 on   # ERROR: GPIO99 doesn't exist
```

### WiFi Validation

```bash
wifi connect "" password  # ERROR: SSID cannot be empty
```

---

## Error Handling in Scripts

Script execution continues even if individual commands fail. Errors are logged with context:

```
--- Script Execution Start ---
[ERROR] gpio 99 on: Invalid pin
[OK] GPIO list displayed
[ERROR] wifi connect: SSID cannot be empty
[OK] System info retrieved
--- Script Complete: 4 lines, 2 errors ---
```

Check the error messages to debug your scripts. All status codes are logged with human-readable messages.

## App Manager (Script Installation)

The `AppManager` class handles installation and execution of `.hx` scripts on LittleFS:

```cpp
#include "api/app_manager.h"
using namespace harixos::api;

// Install a script to /apps/<name>.hx
ApiResult result = AppManager::installApp("myapp", "print Hello\ndelay 1000");

// List all installed apps
AppManager::listApps(Serial);

// Run an installed app
ApiResult result = AppManager::runApp("myapp", Serial);

// Uninstall an app
ApiResult result = AppManager::uninstallApp("myapp");
```

**How it works:**
1. Apps are stored as `.hx` text files in `/apps/` directory on LittleFS
2. The script engine reads and executes each line as a command
3. Results and errors are logged to Serial output
4. Apps can be created at runtime via the interactive `run install` command

---

## Extending the API for New Commands

To add new commands to `.hx` scripts:

1. **Add a handler method** to `ScriptEngine` (in `script_engine.h/.cpp`):

```cpp
// script_engine.h
private:
  static ApiResult handleCustomCommand(const String &args, Stream &output);

// script_engine.cpp
ApiResult ScriptEngine::handleCustomCommand(const String &args, Stream &output) {
  // Parse args and implement logic
  if (args.length() == 0) {
    return ApiResult(API_INVALID_ARGUMENT, "Usage: custom <param>");
  }
  // Your implementation here
  output.println("Custom command executed");
  return ApiResult(API_OK, "Custom command done");
}
```

2. **Register in `executeCommand()`**:

```cpp
// In ScriptEngine::executeCommand()
else if (cmd.name == "custom") {
  return handleCustomCommand(cmd.args, output);
}
```

3. **Update `printHelp()`** to document the new command:

```cpp
output.println(F("  custom <param>        Your custom command"));
```

4. **Update this API documentation** to include usage examples.

---

## CLI Integration

The HarixOS shell exposes API functions as commands:

```bash
HarixOS> gpio 2 on
GPIO2 -> ON

HarixOS> wifi scan
Scanning...
1. Network1 (RSSI: -45, Channel: 1, SECURE)
2. Network2 (RSSI: -67, Channel: 6, OPEN)

HarixOS> run /scripts/blink.hx
--- Script Execution Start ---
[OK] GPIO2 mode set
[OK] Delayed 500 ms
...
```

---

## External App Development

Apps written for HarixOS should:

1. **Use the API** - Never access hardware directly
2. **Check return values** - Handle errors gracefully
3. **Respect boot safety** - Don't force GPIO0/GPIO2 LOW
4. **Manage resources** - Monitor heap, use delays to yield
5. **Follow conventions** - Use the same patterns as built-in commands

---

## Future API Expansion

Planned API modules:

- **I2C API**: Device communication
- **SPI API**: Sensor interfaces
- **HTTP API**: Web requests (GET/POST)
- **Storage API**: File encryption, compression
- **Task Scheduler**: Multi-task simulation
- **Memory Manager**: Heap allocation tracking

---

See [Getting Started](./Getting-Started.md) for basic usage.
