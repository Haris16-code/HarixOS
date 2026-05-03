#ifndef HARIXOS_APP_MANAGER_H
#define HARIXOS_APP_MANAGER_H

#include "api_types.h"
#include "script_engine.h"
#include <Arduino.h>

namespace harixos {
namespace api {

// ===== App Manager =====
// Manages installation and execution of .hx apps/scripts
// Apps are stored as .hx files in /apps/ directory
class AppManager {
public:
  // Install app from source content to /apps/<name>.hx
  static ApiResult installApp(const String &name, const String &content);
  
  // List installed apps
  static ApiResult listApps(Stream &output);
  
  // Run a .hx app/script (can be .hx file or inline script)
  static ApiResult runApp(const String &path, Stream &output);
  
  // Uninstall an app
  static ApiResult uninstallApp(const String &name);
  
  // Get app info
  static ApiResult getAppInfo(const String &name, Stream &output);
  
private:
  static const char *APP_DIR;
};

}  // namespace api
}  // namespace harixos

#endif
