#include "script_engine.h"
#include "gpio_api.h"
#include "wifi_api.h"
#include "system_api.h"
#include "../kernel/filesystem/filesystem.h"

namespace harixos {
namespace api {

ScriptEngine::Command ScriptEngine::parseCommand(const String &line) {
  String trimmed = line;
  trimmed.trim();
  
  if (trimmed.length() == 0) {
    return {"", ""};
  }
  
  int spacePos = trimmed.indexOf(' ');
  if (spacePos == -1) {
    return {trimmed, ""};
  }
  
  return {trimmed.substring(0, spacePos), trimmed.substring(spacePos + 1)};
}

ApiResult ScriptEngine::handleGpioCommand(const String &args, Stream &output) {
  // gpio <pin> <action> [param]
  // Examples:
  //   gpio 2 on
  //   gpio 2 off
  //   gpio 2 read
  //   gpio 2 mode output
  //   gpio 2 pulse 5 500
  
  int space1 = args.indexOf(' ');
  if (space1 == -1) {
    return ApiResult(API_INVALID_ARGUMENT, "Usage: gpio <pin> <on|off|read|mode|pulse|list>");
  }
  
  String pinStr = args.substring(0, space1);
  String rest = args.substring(space1 + 1);
  
  uint8_t pin = pinStr.toInt();
  
  int space2 = rest.indexOf(' ');
  String action = (space2 == -1) ? rest : rest.substring(0, space2);
  String param = (space2 == -1) ? "" : rest.substring(space2 + 1);
  
  action.toLowerCase();
  
  if (action == "on" || action == "1") {
    return GpioAPI::write(pin, HIGH);
  } else if (action == "off" || action == "0") {
    return GpioAPI::write(pin, LOW);
  } else if (action == "read") {
    GpioResult result = GpioAPI::read(pin);
    output.println(result.message);
    return result;
  } else if (action == "mode") {
    param.toLowerCase();
    uint8_t mode = INPUT;
    if (param == "output" || param == "out") mode = OUTPUT;
    else if (param == "input_pullup" || param == "pullup") mode = INPUT_PULLUP;
    return GpioAPI::setMode(pin, mode);
  } else if (action == "pulse") {
    int count = param.toInt();
    if (count <= 0) count = 1;
    return GpioAPI::pulse(pin, count, 250);
  } else if (action == "toggle") {
    return GpioAPI::toggle(pin);
  } else if (action == "list") {
    GpioAPI::listAvailablePins(output);
    return ApiResult(API_OK, "GPIO list displayed");
  } else {
    return ApiResult(API_INVALID_ARGUMENT, "Unknown GPIO action: " + action);
  }
}

ApiResult ScriptEngine::handleWifiCommand(const String &args, Stream &output) {
  // wifi <action> [params]
  // Examples:
  //   wifi scan
  //   wifi connect <ssid> <password>
  //   wifi disconnect
  //   wifi status
  
  int space = args.indexOf(' ');
  String action = (space == -1) ? args : args.substring(0, space);
  String rest = (space == -1) ? "" : args.substring(space + 1);
  
  action.toLowerCase();
  
  if (action == "scan") {
    return WiFiAPI::scan(output);
  } else if (action == "connect") {
    int spacePos = rest.indexOf(' ');
    if (spacePos == -1) {
      return ApiResult(API_INVALID_ARGUMENT, "Usage: wifi connect <ssid> <password>");
    }
    String ssid = rest.substring(0, spacePos);
    String password = rest.substring(spacePos + 1);
    return WiFiAPI::connect(ssid, password);
  } else if (action == "disconnect") {
    return WiFiAPI::disconnect();
  } else if (action == "status") {
    return WiFiAPI::status(output);
  } else if (action == "ip") {
    return WiFiAPI::getIP(output);
  } else {
    return ApiResult(API_INVALID_ARGUMENT, "Unknown WiFi action: " + action);
  }
}

ApiResult ScriptEngine::handleDelayCommand(const String &args, Stream &output) {
  // delay <milliseconds>
  uint32_t ms = args.toInt();
  if (ms == 0) {
    return ApiResult(API_INVALID_ARGUMENT, "Delay must be > 0");
  }
  SystemAPI::delay(ms);
  return ApiResult(API_OK, "Delayed " + String(ms) + " ms");
}

ApiResult ScriptEngine::handleSystemCommand(const String &args, Stream &output) {
  // system <action>
  // Examples:
  //   system info
  //   system heap
  //   system reboot
  
  String action = args;
  action.toLowerCase();
  
  if (action == "info") {
    return SystemAPI::printInfo(output);
  } else if (action == "heap") {
    output.printf("Heap free: %u bytes\r\n", SystemAPI::getHeapFree());
    return ApiResult(API_OK, "Heap info displayed");
  } else if (action == "reboot") {
    SystemAPI::reboot();
    return ApiResult(API_OK, "Rebooting...");
  } else {
    return ApiResult(API_INVALID_ARGUMENT, "Unknown system action");
  }
}

ApiResult ScriptEngine::handlePrintCommand(const String &args, Stream &output) {
  // print <text>
  if (args.length() == 0) {
    output.println();
  } else {
    output.println(args);
  }
  return ApiResult(API_OK, "");
}

ApiResult ScriptEngine::handleRunCommand(const String &args, Stream &output) {
  // run <path>
  String path = args;
  path.trim();
  if (path.length() == 0) {
    return ApiResult(API_INVALID_ARGUMENT, "Usage: run <path>");
  }
  
  String content = harixos::readText(path);
  if (content.length() == 0) {
    return ApiResult(API_ERROR, "Failed to read script or script empty: " + path);
  }
  
  return executeScript(content, output);
}

ApiResult ScriptEngine::executeCommand(const String &command, Stream &output) {
  Command cmd = parseCommand(command);
  
  if (cmd.name.length() == 0) {
    return ApiResult(API_OK, "");  // Empty line
  }
  
  cmd.name.toLowerCase();
  
  if (cmd.name == "print") {
    return handlePrintCommand(cmd.args, output);
  } else if (cmd.name == "gpio") {
    return handleGpioCommand(cmd.args, output);
  } else if (cmd.name == "wifi") {
    return handleWifiCommand(cmd.args, output);
  } else if (cmd.name == "delay") {
    return handleDelayCommand(cmd.args, output);
  } else if (cmd.name == "system") {
    return handleSystemCommand(cmd.args, output);
  } else if (cmd.name == "run") {
    return handleRunCommand(cmd.args, output);
  } else if (cmd.name.startsWith("/") || cmd.name.endsWith(".hx")) {
    // Treat as script path if it looks like one
    return handleRunCommand(command, output);
  } else if (cmd.name == "help") {
    printHelp(output);
    return ApiResult(API_OK, "Help displayed");
  } else if (cmd.name == "#") {
    return ApiResult(API_OK, "");  // Comment line
  } else {
    return ApiResult(API_INVALID_ARGUMENT, "Unknown command: " + cmd.name);
  }
}

ApiResult ScriptEngine::executeScript(const String &script, Stream &output) {
  // Split script into lines and execute each
  int startIdx = 0;
  int lineCount = 0;
  int errorCount = 0;
  int scriptLen = script.length();
  
  output.println(F("--- Script Execution Start ---"));
  
  while (startIdx < scriptLen) {
    int endIdx = script.indexOf('\n', startIdx);
    if (endIdx == -1) {
      endIdx = scriptLen;
    }
    
    String line = script.substring(startIdx, endIdx);
    line.trim();
    
    if (line.length() > 0 && !line.startsWith("#")) {
      ApiResult result = executeCommand(line, output);
      if (result.isError()) {
        output.printf("[ERROR] %s: %s\r\n", line.c_str(), result.message.c_str());
        ++errorCount;
      } else if (result.message.length() > 0) {
        output.printf("[OK] %s\r\n", result.message.c_str());
      }
      ++lineCount;
    }
    
    startIdx = endIdx + 1;
  }
  
  output.printf("--- Script Complete: %d lines, %d errors ---\r\n", lineCount, errorCount);
  
  return errorCount == 0 ? ApiResult(API_OK, "Script executed") 
                         : ApiResult(API_ERROR, String(errorCount) + " errors");
}

void ScriptEngine::printHelp(Stream &output) {
  output.println(F(".hx Script Commands:"));
  output.println();
  output.println(F("Output:"));
  output.println(F("  print <text>            Print text to console"));
  output.println();
  output.println(F("GPIO:"));
  output.println(F("  gpio <pin> on           Set GPIO HIGH"));
  output.println(F("  gpio <pin> off          Set GPIO LOW"));
  output.println(F("  gpio <pin> read         Read GPIO value"));
  output.println(F("  gpio <pin> toggle       Toggle GPIO"));
  output.println(F("  gpio <pin> mode <in|out|pullup>  Set pin mode"));
  output.println(F("  gpio <pin> pulse <count>  Pulse GPIO"));
  output.println(F("  gpio list               Show available pins"));
  output.println();
  output.println(F("WiFi:"));
  output.println(F("  wifi scan               Scan networks"));
  output.println(F("  wifi connect <ssid> <pass>  Connect"));
  output.println(F("  wifi disconnect         Disconnect"));
  output.println(F("  wifi status             Show status"));
  output.println(F("  wifi ip                 Show IP info"));
  output.println();
  output.println(F("System:"));
  output.println(F("  delay <ms>              Sleep (ms)"));
  output.println(F("  system info             Show system info"));
  output.println(F("  system heap             Show heap free"));
  output.println(F("  system reboot           Reboot device"));
  output.println();
  output.println(F("Other:"));
  output.println(F("  # comment               Script comment"));
  output.println(F("  help                    Show this help"));
}

}  // namespace api
}  // namespace harixos
