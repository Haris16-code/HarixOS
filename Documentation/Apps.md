Developing System Apps for HarixOS
==========================

App layout
- Put apps under `src/apps/<appname>/` with a `.h` and `.cpp`.
- Each app should expose a C++ namespace, e.g. `harixos::notepad::run()`.

Example
- See `src/apps/notepad` for a working example.

Registering commands
- `main.cpp` dispatches commands to app handlers. Add new commands to `executeCommand()`.
- Keep app code modular and use utilities in `src/utils/` for common tasks.