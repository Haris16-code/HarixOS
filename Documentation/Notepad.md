Using the HarixOS Notepad App
=============================

Launch
- `notepad /path/to/file.txt` — opens the file for editing. If the file doesn't exist, it will be created when saved.

Editor commands (line input)
- Type lines normally to append them to the buffer.
- `:w` — Save buffer to file.
- `:q` — Quit without saving.
- `:wq` or `:x` — Save and quit.
- `:p` — Print current buffer to console.
- `:d <line>` — Delete line number.

Example
1. `notepad /notes/todo.txt`
2. Enter text lines. Use `:wq` to save and exit.

Implementation
- The notepad app `harixos::runNotepad(path)` is in [src/apps/notepad/notepad.cpp](src/apps/notepad/notepad.cpp#L1).
- It uses filesystem helpers to automically write files.