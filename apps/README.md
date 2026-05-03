# HarixOS .hx Test Apps

This directory contains example `.hx` scripts to test HarixOS functionality.

## Available Test Apps

### hello.hx
Simple demonstration app that prints system information.
```
run hello
```

### sysinfo.hx
Displays detailed system and heap information.
```
run sysinfo
```

### gpio_test.hx
Tests GPIO operations:
- Lists available pins
- Sets GPIO2 output mode
- Tests HIGH/LOW writes
- Tests pin reading
- Tests pulse/toggle operations
```
run gpio_test
```

### wifi_test.hx
Tests WiFi functionality:
- Shows current WiFi status
- Displays IP information
- Scans for available networks
```
run wifi_test
```

## Installing Custom Apps

You can install your own `.hx` apps using the interactive `run install` command:

```
run install myapp
```

Then paste your script content line by line and type `END` on a new line to finish.

## API Commands Available in .hx Scripts

### Print
```
print <text>
```

### GPIO
```
gpio <pin> on|off|read|list|mode <input|output|pullup>|pulse <count>|toggle
```

### WiFi
```
wifi scan|status|connect <ssid> <password>|disconnect|ip
```

### System
```
system info|heap|reboot
```

### Delay
```
delay <milliseconds>
```
