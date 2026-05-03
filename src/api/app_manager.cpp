#include "app_manager.h"
#include <LittleFS.h>

namespace harixos {
namespace api {

const char *AppManager::APP_DIR = "/apps";

ApiResult AppManager::installApp(const String &name, const String &content) {
  if (name.length() == 0) {
    return ApiResult(API_INVALID_ARGUMENT, "App name cannot be empty");
  }
  
  if (content.length() == 0) {
    return ApiResult(API_INVALID_ARGUMENT, "App content cannot be empty");
  }
  
  // Ensure /apps directory exists
  if (!LittleFS.exists(APP_DIR)) {
    if (!LittleFS.mkdir(APP_DIR)) {
      return ApiResult(API_ERROR, "Failed to create /apps directory");
    }
  }
  
  // Write app file
  String appPath = String(APP_DIR) + "/" + name + ".hx";
  File f = LittleFS.open(appPath, "w");
  if (!f) {
    return ApiResult(API_ERROR, "Failed to create app file");
  }
  
  f.print(content);
  f.close();
  
  return ApiResult(API_OK, "App '" + name + "' installed at " + appPath);
}

ApiResult AppManager::listApps(Stream &output) {
  if (!LittleFS.exists(APP_DIR)) {
    output.println(F("No apps installed."));
    return ApiResult(API_OK, "Apps directory is empty");
  }
  
  Dir dir = LittleFS.openDir(APP_DIR);
  int count = 0;
  
  output.println(F("Installed apps:"));
  
  while (dir.next()) {
    if (dir.isFile()) {
      String name = dir.fileName();
      // Remove leading /
      if (name.startsWith("/")) {
        name = name.substring(1);
      }
      // Remove .hx extension
      if (name.endsWith(".hx")) {
        name = name.substring(0, name.length() - 3);
      }
      
      size_t size = dir.fileSize();
      output.printf("  %s (%u bytes)\r\n", name.c_str(), size);
      ++count;
    }
  }
  
  if (count == 0) {
    output.println(F("  (none)"));
  }
  
  return ApiResult(API_OK, String(count) + " app(s) installed");
}

ApiResult AppManager::runApp(const String &path, Stream &output) {
  if (path.length() == 0) {
    return ApiResult(API_INVALID_ARGUMENT, "App path cannot be empty");
  }
  
  // Normalize path
  String fullPath = path;
  if (!fullPath.startsWith("/")) {
    fullPath = "/" + fullPath;
  }
  
  // Check if file exists
  if (!LittleFS.exists(fullPath)) {
    return ApiResult(API_FILE_NOT_FOUND, "App not found: " + fullPath);
  }
  
  // Read file content
  File f = LittleFS.open(fullPath, "r");
  if (!f) {
    return ApiResult(API_ERROR, "Failed to open app file");
  }
  
  String script = "";
  while (f.available()) {
    script += (char)f.read();
  }
  f.close();
  
  // Execute script
  output.println();
  output.printf("Running app: %s\r\n", path.c_str());
  output.println(F("---"));
  
  ApiResult result = ScriptEngine::executeScript(script, output);
  
  output.println(F("---"));
  
  return result;
}

ApiResult AppManager::uninstallApp(const String &name) {
  if (name.length() == 0) {
    return ApiResult(API_INVALID_ARGUMENT, "App name cannot be empty");
  }
  
  String appPath = String(APP_DIR) + "/" + name + ".hx";
  
  if (!LittleFS.exists(appPath)) {
    return ApiResult(API_FILE_NOT_FOUND, "App not found: " + name);
  }
  
  if (!LittleFS.remove(appPath)) {
    return ApiResult(API_ERROR, "Failed to remove app");
  }
  
  return ApiResult(API_OK, "App '" + name + "' uninstalled");
}

ApiResult AppManager::getAppInfo(const String &name, Stream &output) {
  if (name.length() == 0) {
    return ApiResult(API_INVALID_ARGUMENT, "App name cannot be empty");
  }
  
  String appPath = String(APP_DIR) + "/" + name + ".hx";
  
  if (!LittleFS.exists(appPath)) {
    return ApiResult(API_FILE_NOT_FOUND, "App not found: " + name);
  }
  
  File f = LittleFS.open(appPath, "r");
  if (!f) {
    return ApiResult(API_ERROR, "Failed to open app file");
  }
  
  size_t size = f.size();
  f.close();
  
  output.printf("App: %s\r\n", name.c_str());
  output.printf("  Path: %s\r\n", appPath.c_str());
  output.printf("  Size: %u bytes\r\n", size);
  
  return ApiResult(API_OK, "App info retrieved");
}

}  // namespace api
}  // namespace harixos
