# HarixOS v1.0 .hx Script Quick Reference

## New Commands

### About Command

Shows HarixOS version and information:

```bash
about
```

Output includes:
- Version number
- Description
- Features list
- Build date and time

### Pull Command (Download Files)

Download files from the internet:

```bash
pull http://example.com/file.bin /data/file.bin
pull https://example.com/data.txt /scripts/data.txt
```

**Requirements:**
- Board must be connected to WiFi
- Supports HTTP and HTTPS
- Saves to LittleFS path

**Example:**
```bash
# First connect to WiFi
wifi connect 'MyNetwork' 'MyPassword'

# Then download a file
pull http://example.com/myapp.hx /apps/myapp.hx

# Run the downloaded app
run myapp
```

**WiFi Automation:**
- Once connected, WiFi stays connected until `wifi disconnect`
- Credentials are saved; HarixOS auto-connects to saved networks at boot
- Automatically prompted if WiFi is disconnected

### Update Command (System Updates)

Check for new HarixOS versions from GitHub:

```bash
update check
```

**Features:**
- Shows new version number
- Displays changelog/What's New
- Provides direct download link

**Automatic Boot Check:**
HarixOS can automatically check for updates on boot if WiFi is connected. This is disabled by default.
- **Enable:** `settings update on`
- **Disable:** `settings update off` (default)
- **Check Manually:** `update check`

## Print Command (Output)

```bash
print Hello, World!        # Print text
print                      # Print blank line
print GPIO value: 1
print WiFi: Connected
```

## GPIO Command (Hardware Control)

### Set Pin State
```bash
gpio 2 on                  # Set GPIO2 HIGH
gpio 2 off                 # Set GPIO2 LOW
gpio 2 toggle              # Toggle GPIO2
```

### Pin Configuration
```bash
gpio 2 mode output         # Set GPIO2 as output
gpio 2 mode input          # Set GPIO2 as input
gpio 2 mode input_pullup   # Set GPIO2 as input with pullup
```

### Read and Test
```bash
gpio 2 read                # Read GPIO2 value (HIGH/LOW)
gpio 2 pulse 5             # Pulse GPIO2 5 times (500ms default)
gpio 2 pulse 10 250        # Pulse GPIO2 10 times, 250ms between
gpio list                  # Show available GPIO pins on this board
```

### ESP-01 Available Pins
```bash
gpio 0                     # Available (boot pin - HIGH required)
gpio 2                     # Available (boot pin - HIGH required)
# All other pins not exposed on ESP-01
```

### ESP8266 (4MB+) Available Pins
```bash
gpio 0                     # Available (HIGH required at boot)
gpio 2                     # Available (HIGH required at boot)
gpio 4                     # Available
gpio 5                     # Available
gpio 12                    # Available
gpio 13                    # Available
gpio 14                    # Available
gpio 15                    # Available (LOW required at boot)
gpio 16                    # Available
```

## WiFi Command (Networking)

```bash
wifi scan                  # Scan available networks
wifi connect 'SSID' 'pass' # Connect to WiFi (use quotes)
wifi disconnect            # Disconnect from WiFi
wifi status                # Show WiFi status
wifi ip                    # Show IP configuration
```

## Time & Schedule Commands (Automation)

### System Clock
```bash
time                       # Show current time
time sync <TZ_STRING>      # Sync via NTP (e.g., PKT-5)
time list-tz               # Show timezone examples
```

### Task Scheduler
```bash
schedule list              # List active tasks
schedule remove <id>       # Delete task
schedule add 14:30:00 'gpio 2 toggle' # Daily task
schedule add +30s 'reboot' # One-off delayed task
```

## Delay Command (Timing)

```bash
delay 1000                 # Wait 1000 milliseconds (1 second)
delay 500                  # Wait 500 milliseconds (0.5 seconds)
delay 100                  # Wait 100 milliseconds
```

## System Command (Device Info)

```bash
system info                # Show chip ID, core version, SDK, etc.
system heap                # Show free heap memory
system reboot              # Reboot the device
```

## Comments and Help

```bash
# This is a comment
# Comments start with # and are ignored
help                       # Show available commands
```

## Common Patterns

### Blink LED (GPIO2)
```bash
print Setting up GPIO2...
gpio 2 mode output
print GPIO2 ready

print Blinking 5 times...
gpio 2 pulse 5 500

print Blink complete!
```

### Check System
```bash
print === System Check ===
system info
print
print Free memory:
system heap
print === Done ===
```

### WiFi Check
```bash
print Checking WiFi...
wifi status
print
print IP Configuration:
wifi ip
print
print Scanning networks...
wifi scan
```

### LED Sequence
```bash
# Turn LED ON
gpio 2 on
delay 1000

# Turn LED OFF
gpio 2 off
delay 1000

# Turn LED ON again
gpio 2 on
delay 1000

# Done
print Sequence complete
```

### Safe GPIO Test (No Boot Pins)
```bash
# Test GPIO2 (safe on ESP-01)
print Testing GPIO2...
gpio 2 mode output
gpio 2 on
delay 500
gpio 2 off
delay 500

# Test GPIO4 (if available)
print Testing GPIO4...
gpio 4 mode output
gpio 4 on
delay 500
gpio 4 off
print Tests done!
```

## Error Examples (and how to avoid them)

### ❌ Invalid GPIO Pin
```bash
gpio 99 on     # ERROR: GPIO99 doesn't exist
```
**Fix:** Use valid pin (GPIO0, GPIO2, GPIO4, GPIO5, GPIO12, GPIO13, GPIO14, GPIO15, GPIO16)

### ❌ Boot Pin Issue
```bash
gpio 0 off     # ERROR: GPIO0 must stay HIGH
gpio 2 off     # ERROR: GPIO2 must stay HIGH
```
**Fix:** These pins are required for boot. Don't set them LOW.

### ❌ Empty SSID
```bash
wifi connect "" password   # ERROR: SSID cannot be empty
```
**Fix:** Provide valid SSID

### ❌ Invalid Delay
```bash
delay 0        # ERROR: Delay must be > 0
delay -100     # ERROR: Delay must be > 0
```
**Fix:** Use positive milliseconds (delay 100)

## Installing and Running Scripts

### Step 1: Install
```bash
run install myapp
```

### Step 2: Enter Script Content
```
print Hello from my app!
gpio 2 mode output
gpio 2 on
delay 1000
gpio 2 off
print Done!
END
```
(Type `END` on new line to finish)

### Step 3: Run
```bash
run myapp
```

### Step 4: List Apps
```bash
run list
```

### Step 5: Uninstall
```bash
run uninstall myapp
```

## Output Format

**Successful Command:**
```
[OK] Command description
```

**Failed Command:**
```
[ERROR] command line: Error message
```

**Script Summary:**
```
--- Script Execution Start ---
[OK] 
[ERROR] gpio 99 on: Invalid pin
[OK] 
--- Script Complete: 3 lines, 1 errors ---
```

## Best Practices

✅ **DO:**
- Test commands individually first
- Use print statements for status
- Check GPIO availability (run `gpio list`)
- Add delays between GPIO operations
- Comment your scripts with #

❌ **DON'T:**
- Force GPIO0/GPIO2 LOW (breaks boot)
- Use unavailable pins
- Forget END marker when installing
- Use delay 0 or negative delays
- Assume operations are instant

## Memory and Performance

- **Heap:** ~47KB available on ESP-01
- **Script max lines:** ~1000 (depends on heap)
- **GPIO operation time:** <1ms
- **WiFi scan:** 5-30 seconds
- **Reboot time:** ~2 seconds

## Debug Tips

1. **Check available pins:**
   ```bash
   gpio list
   ```

2. **Check system status:**
   ```bash
   system info
   system heap
   ```

3. **Check WiFi:**
   ```bash
   wifi status
   wifi ip
   ```

4. **Add print statements** to see script progress:
   ```bash
   print Starting operation...
   gpio 2 on
   print GPIO2 is HIGH
   ```

5. **Read error messages** - they tell you what's wrong!

## Example: Complete App

```bash
print ===========================
print My HarixOS App v1.0
print ===========================
print

print 1. Checking system...
system info
delay 500

print
print 2. Testing GPIO2...
gpio 2 mode output
gpio 2 on
print GPIO2 is HIGH
delay 1000
gpio 2 off
print GPIO2 is LOW

print
print 3. WiFi Status...
wifi status

print
print ===========================
print App complete!
print ===========================
```

Save this as `run install myapp` and paste the script!
