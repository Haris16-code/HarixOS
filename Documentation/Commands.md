HarixOS Shell Commands
=======================

Core shell commands
- `help` — Show help and command list
- `info` — Show system, flash and memory information
- `reboot` — Restart the device
- `clear` — Clear serial console (emulator)
- `update check` — Check for system updates via GitHub
- `pull <url> <path>` — Download file from the internet (HTTP/HTTPS)
- `time` — Show time and NTP sync options
- `schedule` — Manage background tasks (CRON-like)

Filesystem (`fs`) commands
- `pwd` — Print current working directory
- `ls [path]` — List directory contents
- `cd <path>` — Change directory (supports `..` and absolute paths)
- `mkdir <path>` — Create directory recursively
- `touch <path>` — Create empty file
- `cat <file>` — Display file contents
- `write <file> <text...>` — Overwrite file with text
- `append <file> <text...>` — Append text to file
- `rm <path>` — Remove file or empty directory
- `cp <src> <dst>` — Copy files (not recursive)
- `mv <src> <dst>` — Move/rename files
- `format` — Format LittleFS filesystem (erases data)

Apps
- `notepad <path>` — Open interactive line editor
- `settings` — Show and edit persistent shell settings (e.g., `settings update on|off`)
- `calc <expr>` — Evaluate arithmetic expressions

HTTP file server
- `serve <file> [port]` — Start a simple HTTP server serving the provided file as the index. If not connected to Wi‑Fi the shell will prompt for SSID and password. Returns the device IP and port when started.
- `serve stop` — Stop the HTTP server.
- `serve status` — Show current server status and served file.

Notes: When serving a file (for example `/www/index.html`), linked resources referenced by paths in the HTML (e.g., `/script.js`, `/styles.css`) will also be served from the same directory. The server logs each request to Serial.

Networking
- `wifi scan` — Scan available networks
- `wifi connect '<ssid>' '<pass>'` — Connect to Wi‑Fi (use quotes for SSID/Pass)
- `wifi status` — Show Wi‑Fi status

Hardware
- `gpio read <pin>` — Read GPIO
- `gpio write <pin> <on|off|toggle|0|1>` — Set GPIO output
- `i2c scan` — Scan for I2C devices

Examples
- Create and edit a file:

```
mkdir notes
cd notes
notepad todo.txt
```

- Quick filesystem check:

```
ls /
pwd
cat /harixos/settings.cfg
```
