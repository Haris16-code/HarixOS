Troubleshooting HarixOS
=======================

Build errors
- Missing LittleFS: ensure `#include <LittleFS.h>` and `LittleFS.begin()` present.
- Linker errors: confirm source files are included under `src/` and have proper header guards.

Device fails to boot
- Ensure ESP8266 boot pins are in valid states (GPIO0 HIGH, GPIO2 HIGH, GPIO15 LOW on standard modules).
- If serial logs show `rst:0x10`, the chip was in deep sleep; power-cycle the module.

Filesystem problems
- If files not visible or corrupted, reformat with `format`.
- Use `ls` to verify the structure.

Serial garbled output
- Confirm monitor baud rate 115200.
- Ensure if using ESP-01 make sure connect directly and no usb hub or usb extender is used.

---

## Join HarixOS Community For Discussion
Still having trouble? Ask the community!
- **Discussions**: [HarixOS Community](https://harixos.harislab.tech)
