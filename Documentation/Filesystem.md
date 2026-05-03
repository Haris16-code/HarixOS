LittleFS and HarixOS Filesystem Guide
=====================================

HarixOS uses LittleFS to provide a POSIX-like filesystem interface on ESP8266.

Path semantics
- Paths use `/` as separator. Leading `/` indicates absolute path, otherwise paths are resolved relative to the current working directory.
- `.` and `..` are supported.

Common workflows
- Format storage: `format` — WARNING: erases all files.
- List root: `ls /`
- Create nested directories: `mkdir /notes/2026`
- Move files: `mv /tmp/a.txt /notes/a.txt`

Space limits and best practices
- Filesystem size depends on board flash capacity and partitioning. Keep files small (plain text) on low-flash modules.
- Avoid storing large binaries; use external storage if needed.

Implementation notes
- Files are implemented with LittleFS via `src/utils/filesystem` helpers.
- Path normalization and resolution ensure commands work like Linux-style shells.
