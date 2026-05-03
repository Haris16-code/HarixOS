# HarixOS v1.0 Quick Start

## Getting Started

### 1. Flash Firmware

```bash
cd HarixOS
pio run --target upload
```

### 2. Connect Serial Console

Use PuTTY or similar at **115200 baud**:
- Port: Your ESP8266's serial port
- Baud: 115200
- Data: 8
- Parity: None
- Stop: 1

### 3. Test Basic Commands

```bash
# Check version
about

# Show all commands
help

# Get system info
info
```

### WiFi Connection

```bash
# Connect to WiFi (use quotes for SSID/Pass)
wifi connect 'MyNetwork' 'password'
Connected.
Note: Credentials are saved; HarixOS will auto-connect at boot!

# Check connection
wifi status

# Get IP info
wifi ip

# View multiple times without reconnecting
wifi scan
```

### Download Files (Pull Command)

```bash
# Prerequisites: WiFi connected

# Download a .hx script
pull http://example.com/myapp.hx /apps/myapp.hx
Downloading: http://example.com/myapp.hx
Please wait...
  0 / 1024 bytes
  512 / 1024 bytes
Success! Downloaded 1024 bytes to /apps/myapp.hx

# Run the downloaded app
run myapp
```

### Create and Run .hx Scripts

```bash
# Install new script
run install hello

# Enter script content (paste this):
print Hello from HarixOS v1.0!
print
system info
print
print Done!
END

# Run the script
run hello

# List installed scripts
run list
```

### Version Information

```bash
# Show HarixOS version and features
about

# Output:
# ========================================
#  HarixOS v1.0.0
# ========================================
#
#  ESP8266 command shell with .hx scripting
#
# Features:
#   - ESP8266 command shell
#   - .hx script support
#   - GPIO and WiFi control
#   - File management
#   - HTTP file download
# ...
```

### System Updates

```bash
# Check for new HarixOS versions
update check

# Output:
# Checking for updates...
# Current version: 1.0.0
# Latest version:  1.0.1
#
# [!] A new update is available!
#
# What's New:
#  - Improved WiFi stability
#  - Added update checker
#
# Download new update at:
# https://github.com/Haris16-code/HarixOS/releases/latest
```

## Common Workflows

### 1. Download and Run Remote Script

```bash
# Setup WiFi
wifi connect 'MyNetwork' 'password'

# Download script
pull http://myserver.com/automation.hx /apps/automation.hx

# Run it
run automation
```

### 2. Control LED with Script

```bash
run install blink
print Creating LED blink script...
gpio 2 mode output
print
print Blink 5 times:
gpio 2 pulse 5 500
print Done!
END

run blink
```

### 3. Monitor System Status

```bash
run install monitor
print === System Monitor ===
print
system info
print
print Memory:
system heap
print
print WiFi:
wifi status
print
print Done!
END

run monitor
```

### 4. Download Configuration File

```bash
# Connect to WiFi
wifi connect 'MyNetwork' 'password'

# Download config
pull http://myserver.com/config.txt /data/config.txt

# View it
cat /data/config.txt
```

## Command Reference

### Essential Commands

```bash
about              # Show version info
pull <url> <path>  # Download file
wifi connect 'ID' 'PW'# Connect to WiFi
run install <name> # Create .hx script
update check       # Check for updates
help               # Show all commands
```

### WiFi Management

```bash
wifi status        # Check connection
wifi disconnect    # Disconnect
wifi scan          # Show networks
wifi ip            # Show IP
```

### .hx Script Commands (inside scripts)

```bash
print <text>       # Output text
gpio <pin> <action># Control GPIO
delay <ms>         # Wait milliseconds
system info        # Show system info
wifi scan          # Scan networks
```

## Troubleshooting

### "Not connected to WiFi" error with pull

```bash
# Check status
wifi status

# If disconnected, reconnect
wifi connect 'MyNetwork' 'password'

# Show available networks
wifi scan
```

### Script not found when running

```bash
# List installed scripts
run list

# If empty, install first
run install myapp
```

### Download fails with timeout

- Check internet connection
- Try with smaller file
- Verify URL is correct
- Check free space: `fs ls /`

### Commands not working

1. Restart device: `reboot`
2. Check firmware version: `about`
3. Try basic command: `info`
4. Read help: `help <topic>`

## Tips & Tricks

### Persistent WiFi

WiFi stays connected once you connect:
```bash
wifi connect 'MyNetwork' 'password'  # Just once!
pull http://site1.com/file1.hx /apps/f1.hx
pull http://site2.com/file2.hx /apps/f2.hx
pull http://site3.com/file3.hx /apps/f3.hx
# No need to reconnect!
```

### Script Organization

Keep scripts in `/apps/`:
```bash
run list                    # See what's installed
run install newscript       # Install
run newscript              # Run
run uninstall oldscript    # Remove
```

### GPIO Safety

ESP-01 board has two GPIO pins:
```bash
gpio list  # Shows available pins
# GPIO0 and GPIO2 on ESP-01
# These must stay HIGH during boot!
```

## File Management

### Explore filesystem

```bash
pwd              # Current directory
ls               # List files
ls /apps         # List apps
ls -R            # Recursive listing
cat /apps/app.hx # View script
```

### Create files

```bash
touch /data/myfile.txt
write /data/myfile.txt "content"
append /data/myfile.txt "more"
```

## Getting Help

1. **Command help**: `help` or `help <topic>`
2. **Version info**: `about`
3. **Script docs**: See `/apps/*.hx` examples
4. **Full docs**: Check `Documentation/` folder

## Next Steps

- Explore `.hx` scripts: `run list`
- Download remote apps: `pull http://...`
- Create automation: `run install myapp`
- Build complex apps: See COMPLEXITY-LEVELS.md
