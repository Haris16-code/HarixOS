#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <FS.h>
#include <LittleFS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>

#define HARIXOS_VERSION_MAJOR 1
#define HARIXOS_VERSION_MINOR 0
#define HARIXOS_VERSION_PATCH 0

#define HARIXOS_VERSION_STRING "1.0.0"
#define HARIXOS_BUILD_DATE __DATE__
#define HARIXOS_BUILD_TIME __TIME__

// Version display
#define HARIXOS_NAME "HarixOS"
#define HARIXOS_DESCRIPTION "An Open-Source Operating System For ESP Microcontrollers"
#define HARIXOS_AUTHOR "Haris"
#include "apps/notepad/notepad.h"
#include "apps/settings/settings.h"
#include "kernel/filesystem/filesystem.h"
#include "utils/http/http_downloader.h"
#include "api/app_manager.h"
#include "api/script_engine.h"
#include "kernel/scheduler/scheduler.h"
#include "kernel/cpu_handler/cpu_handler.h"

namespace {

constexpr size_t kMaxLineLength = 160;
constexpr size_t kMaxTokens = 12;
constexpr uint8_t kDefaultApChannel = 1;

String inputLine;
bool promptVisible = false;
String currentWorkingDirectory = "/";
harixos::AppSettings shellSettings;

// Simple HTTP file server
ESP8266WebServer *httpServer = nullptr;
String httpServeFile;
String httpServeDir;
uint16_t httpServePort = 80;
bool httpServerRunning = false;

struct TokenizedLine {
  String tokens[kMaxTokens];
  size_t count = 0;
};

String toLowerCopy(String value) {
  value.toLowerCase();
  return value;
}

String trimCopy(String value) {
  value.trim();
  return value;
}

String bytesToHuman(uint32_t bytes) {
  const char *suffixes[] = {"B", "KB", "MB"};
  float value = static_cast<float>(bytes);
  uint8_t suffix = 0;
  while (value >= 1024.0f && suffix < 2) {
    value /= 1024.0f;
    ++suffix;
  }

  String text = String(value, value < 10.0f && suffix > 0 ? 2 : 0);
  text += ' ';
  text += suffixes[suffix];
  return text;
}

String uptimeToHuman(unsigned long ms) {
  unsigned long seconds = ms / 1000UL;
  unsigned long days = seconds / 86400UL;
  seconds %= 86400UL;
  unsigned long hours = seconds / 3600UL;
  seconds %= 3600UL;
  unsigned long minutes = seconds / 60UL;
  seconds %= 60UL;

  String out;
  if (days > 0) {
    out += String(days);
    out += "d ";
  }
  if (days > 0 || hours > 0) {
    out += String(hours);
    out += "h ";
  }
  if (days > 0 || hours > 0 || minutes > 0) {
    out += String(minutes);
    out += "m ";
  }
  out += String(seconds);
  out += 's';
  return out;
}

String flashModeToString(FlashMode_t mode) {
  switch (mode) {
    case FM_QIO: return F("QIO");
    case FM_QOUT: return F("QOUT");
    case FM_DIO: return F("DIO");
    case FM_DOUT: return F("DOUT");
    case FM_UNKNOWN:
    default: return F("UNKNOWN");
  }
}

String wifiModeToString(WiFiMode_t mode) {
  switch (mode) {
    case WIFI_OFF: return F("OFF");
    case WIFI_STA: return F("STA");
    case WIFI_AP: return F("AP");
    case WIFI_AP_STA: return F("AP+STA");
    default: return F("UNKNOWN");
  }
}

String wifiStatusToString(wl_status_t status) {
  switch (status) {
    case WL_NO_SHIELD: return F("NO_SHIELD");
    case WL_IDLE_STATUS: return F("IDLE");
    case WL_NO_SSID_AVAIL: return F("NO_SSID");
    case WL_SCAN_COMPLETED: return F("SCAN_DONE");
    case WL_CONNECTED: return F("CONNECTED");
    case WL_CONNECT_FAILED: return F("CONNECT_FAILED");
    case WL_CONNECTION_LOST: return F("CONNECTION_LOST");
    case WL_DISCONNECTED: return F("DISCONNECTED");
    default: return F("UNKNOWN");
  }
}

String resetReasonToString() {
  return ESP.getResetReason();
}

bool isAvailableGpio(uint8_t pin) {
  // ESP8266 GPIO6-11 are connected to flash; GPIO1/3 are UART TX/RX.
  if (pin > 16) {
    return false;
  }
  if (pin >= 6 && pin <= 11) {
    return false;
  }
  if (pin == 1 || pin == 3) {
    return false;
  }
  return true;
}

bool isBootStrapPin(uint8_t pin) {
  return pin == 0 || pin == 2 || pin == 15;
}

bool isUnsafeBootLevel(uint8_t pin, int level) {
  // ESP8266 boot straps: GPIO0=HIGH, GPIO2=HIGH, GPIO15=LOW.
  if (pin == 0 || pin == 2) {
    return level == LOW;
  }
  if (pin == 15) {
    return level == HIGH;
  }
  return false;
}

void prepareBootStrapPinsForReset() {
  // Do nothing - let bootloader handle boot straps.
  // Any GPIO manipulation can trigger watchdog on ESP-01.
}

void printPrompt() {
  Serial.print(F("HarixOS> "));
  promptVisible = true;
}

TokenizedLine tokenize(const String &line) {
  TokenizedLine result;
  const size_t length = line.length();
  size_t index = 0;

  while (index < length && result.count < kMaxTokens) {
    while (index < length && isspace(static_cast<unsigned char>(line[index]))) {
      ++index;
    }
    if (index >= length) {
      break;
    }

    String token;
    char quoteChar = 0;
    if (line[index] == '"' || line[index] == '\'') {
      quoteChar = line[index++];
      while (index < length && line[index] != quoteChar) {
        token += line[index++];
      }
      if (index < length && line[index] == quoteChar) {
        ++index;
      }
    } else {
      while (index < length && !isspace(static_cast<unsigned char>(line[index]))) {
        token += line[index++];
      }
    }

    if (token.length() > 0) {
      result.tokens[result.count++] = token;
    }
  }

  return result;
}

bool parsePin(const String &value, uint8_t &pin) {
  if (value.length() == 0) {
    return false;
  }

  long parsed = value.toInt();
  if (parsed < 0 || parsed > 255) {
    return false;
  }

  pin = static_cast<uint8_t>(parsed);
  return true;
}

void printSystemInfo() {
  Serial.println(F("System"));
  Serial.printf("  Chip ID: 0x%08X\r\n", ESP.getChipId());
  Serial.printf("  Core version: %s\r\n", ESP.getCoreVersion().c_str());
  Serial.printf("  SDK version: %s\r\n", ESP.getSdkVersion());
  Serial.printf("  CPU frequency: %u MHz\r\n", ESP.getCpuFreqMHz());
  Serial.printf("  Reset reason: %s\r\n", resetReasonToString().c_str());
  Serial.printf("  Uptime: %s\r\n", uptimeToHuman(millis()).c_str());
  Serial.printf("  Heap free: %u bytes\r\n", ESP.getFreeHeap());
  Serial.printf("  Sketch used: %s\r\n", bytesToHuman(ESP.getSketchSize()).c_str());
  Serial.printf("  Free sketch space: %s\r\n", bytesToHuman(ESP.getFreeSketchSpace()).c_str());
  Serial.printf("  Flash size: %s\r\n", bytesToHuman(ESP.getFlashChipRealSize()).c_str());
  Serial.printf("  Flash mode: %s\r\n", flashModeToString(ESP.getFlashChipMode()).c_str());
  Serial.printf("  Flash speed: %lu MHz\r\n", ESP.getFlashChipSpeed() / 1000000UL);
}

void printWifiStatus() {
  Serial.println(F("WiFi"));
  Serial.printf("  Mode: %s\r\n", wifiModeToString(WiFi.getMode()).c_str());
  Serial.printf("  Status: %s\r\n", wifiStatusToString(WiFi.status()).c_str());
  Serial.printf("  Station SSID: %s\r\n", WiFi.SSID().c_str());
  Serial.printf("  Station IP: %s\r\n", WiFi.localIP().toString().c_str());
  Serial.printf("  Gateway: %s\r\n", WiFi.gatewayIP().toString().c_str());
  Serial.printf("  DNS: %s\r\n", WiFi.dnsIP().toString().c_str());
  Serial.printf("  Subnet: %s\r\n", WiFi.subnetMask().toString().c_str());
  Serial.printf("  RSSI: %d dBm\r\n", WiFi.isConnected() ? WiFi.RSSI() : 0);

  uint8_t mac[6];
  WiFi.macAddress(mac);
  Serial.printf("  Station MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  if (WiFi.softAPmacAddress(mac)) {
    Serial.printf("  AP MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  }
  Serial.printf("  AP IP: %s\r\n", WiFi.softAPIP().toString().c_str());
}

void printGpioHelp() {
  Serial.println(F("GPIO"));
  Serial.println(F("  ESP8266 usable pins: GPIO0,2,4,5,12,13,14,15,16"));
  Serial.println(F("  Reserved: GPIO6-11 (flash), GPIO1/3 (serial RX/TX)"));
  Serial.println(F("  Boot pins: avoid forcing GPIO0/2/15 to invalid levels during reset"));
}

void printGpioPins() {
  Serial.println(F("GPIO"));
  Serial.printf("  Chip ID: 0x%08X (ESP8266 family)\r\n", ESP.getChipId());

  // ESP-01 has only GPIO0 and GPIO2 exposed.
  // Other modules have more pins available.
  uint32_t flashSize = ESP.getFlashChipRealSize();
  bool isEsp01 = (flashSize == 1048576); // 1MB = ESP-01

  if (isEsp01) {
    Serial.println(F("  Board: ESP-01 (1MB flash)"));
    Serial.println(F("  Exposed pins: GPIO0, GPIO2"));
    Serial.println(F("  Reserved: GPIO1/3 (serial RX/TX), GPIO6-11 (flash), others not exposed"));
    Serial.println(F("  Boot straps: GPIO0=HIGH, GPIO2=HIGH (critical for normal boot)"));
    Serial.println(F("  Pin levels:"));
    const uint8_t esp01_pins[] = {0, 2};
    for (uint8_t i = 0; i < sizeof(esp01_pins); ++i) {
      uint8_t pin = esp01_pins[i];
      pinMode(pin, INPUT);
      int level = digitalRead(pin);
      Serial.printf("    GPIO%u = %d [boot: must be HIGH]\r\n", pin, level);
    }
  } else {
    Serial.println(F("  Board: ESP8266 standard (4MB flash typical)"));
    Serial.println(F("  Usable pins: GPIO0, GPIO2, GPIO4, GPIO5, GPIO12, GPIO13, GPIO14, GPIO15, GPIO16"));
    Serial.println(F("  Reserved: GPIO6-11 (flash), GPIO1/3 (serial RX/TX)"));
    Serial.println(F("  Boot straps: GPIO0=HIGH, GPIO2=HIGH, GPIO15=LOW (critical for normal boot)"));
    Serial.println(F("  Pin levels:"));
    const uint8_t std_pins[] = {0, 2, 4, 5, 12, 13, 14, 15, 16};
    for (uint8_t i = 0; i < sizeof(std_pins); ++i) {
      uint8_t pin = std_pins[i];
      pinMode(pin, INPUT);
      int level = digitalRead(pin);
      const char *bootstrap = "";
      if (pin == 0 || pin == 2) bootstrap = " [boot: must be HIGH]";
      else if (pin == 15) bootstrap = " [boot: must be LOW]";
      Serial.printf("    GPIO%u = %d%s\r\n", pin, level, bootstrap);
    }
  }
}

void handleHelp(const TokenizedLine &cmd);
void handleWifi(const TokenizedLine &cmd);
void handleGpio(const TokenizedLine &cmd);
void handleI2c(const TokenizedLine &cmd);
void handleFs(const TokenizedLine &cmd);
void handlePwd();
void handleCd(const TokenizedLine &cmd);
void handleLs(const TokenizedLine &cmd);
void handleMkdir(const TokenizedLine &cmd);
void handleTouch(const TokenizedLine &cmd);
void handleRm(const TokenizedLine &cmd);
void handleCp(const TokenizedLine &cmd);
void handleMv(const TokenizedLine &cmd);
void handleCat(const TokenizedLine &cmd);
void handleWrite(const TokenizedLine &cmd);
void handleAppend(const TokenizedLine &cmd);
void handleNotepad(const TokenizedLine &cmd);
void handleSettings(const TokenizedLine &cmd);
void handleServe(const TokenizedLine &cmd);
void handleHttpStop();
void handlePull(const TokenizedLine &cmd);
void handleUpdate(const TokenizedLine &cmd);
void tryAutoWifi();
String readLineBlocking();

void handleInfo() {
  printSystemInfo();
  printWifiStatus();
  printGpioHelp();
}

void handleAboutInfo() {
  Serial.println();
  Serial.println(F("========================================"));
  Serial.printf(" %s v%s\r\n", HARIXOS_NAME, HARIXOS_VERSION_STRING);
  Serial.println(F("========================================"));
  Serial.println();
  Serial.printf(" %s\r\n", HARIXOS_DESCRIPTION);
  Serial.println();
  Serial.println(F("  More Features Are In Development! So Stay Tuned :) Also Give A Star On GitHub!"));
  Serial.println();
  Serial.println(F("Documentation:"));
  Serial.println(F("  Type 'help' for command reference"));
  Serial.println(F("  Type 'run list' to see installed apps"));
  Serial.println();
  Serial.println(F("License:"));
  Serial.println(F("  Licensed under GNU General Public License v3.0"));
  Serial.println(F("  Copyright (C) 2026 Haris"));
  Serial.println(F("  Full License: https://github.com/Haris16-code/HarixOS/blob/main/LICENSE"));
  Serial.println();
  Serial.printf(" Built: %s %s\r\n", HARIXOS_BUILD_DATE, HARIXOS_BUILD_TIME);
  Serial.println(F("========================================"));
  Serial.println();
}

void handleReset() {
  Serial.println(F("Rebooting..."));
  Serial.flush();
  delay(100);
  prepareBootStrapPinsForReset();
  ESP.restart();
}

void handleUptime() {
  Serial.printf("Uptime: %s\r\n", uptimeToHuman(millis()).c_str());
}

void handleHeap() {
  Serial.printf("Heap free: %u bytes\r\n", ESP.getFreeHeap());
  Serial.println(F("Heap fragmentation is not exposed directly by the core."));
}

void handleChip() {
  Serial.printf("Chip ID: 0x%08X\r\n", ESP.getChipId());
  Serial.printf("CPU frequency: %u MHz\r\n", ESP.getCpuFreqMHz());
  Serial.printf("Reset reason: %s\r\n", resetReasonToString().c_str());
  Serial.printf("Flash size: %s\r\n", bytesToHuman(ESP.getFlashChipRealSize()).c_str());
  Serial.printf("Flash mode: %s\r\n", flashModeToString(ESP.getFlashChipMode()).c_str());
}

void handleAdc() {
  int raw = analogRead(A0);
  Serial.printf("ADC A0: %d / 1023\r\n", raw);
  Serial.println(F("Note: A0 scaling depends on the ESP8266 board design."));
}

// --- Filesystem helpers (cwd-aware LittleFS shell) ---
void handlePwd() {
  Serial.println(currentWorkingDirectory);
}

void handleCd(const TokenizedLine &cmd) {
  if (cmd.count < 2) {
    handlePwd();
    return;
  }

  String target = harixos::resolvePath(currentWorkingDirectory, cmd.tokens[1]);
  if (!harixos::exists(target)) {
    Serial.println(F("cd: no such file or directory"));
    return;
  }
  if (!harixos::isDirectory(target)) {
    Serial.println(F("cd: not a directory"));
    return;
  }

  currentWorkingDirectory = target;
}

void handleLs(const TokenizedLine &cmd) {
  bool recursive = false;
  String path = currentWorkingDirectory;

  if (cmd.count >= 2) {
    if (cmd.tokens[1] == F("-R") || cmd.tokens[1] == F("-r")) {
      recursive = true;
      if (cmd.count >= 3) {
        path = harixos::resolvePath(currentWorkingDirectory, cmd.tokens[2]);
      }
    } else {
      path = harixos::resolvePath(currentWorkingDirectory, cmd.tokens[1]);
      if (cmd.count >= 3 && (cmd.tokens[2] == F("-R") || cmd.tokens[2] == F("-r"))) {
        recursive = true;
      }
    }
  }

  harixos::listDirectory(path, Serial, recursive);
}

void handleMkdir(const TokenizedLine &cmd) {
  if (cmd.count < 2) {
    Serial.println(F("Usage: mkdir <path>"));
    return;
  }

  String path = harixos::resolvePath(currentWorkingDirectory, cmd.tokens[1]);
  if (harixos::makeDirectory(path)) {
    Serial.printf("Created directory: %s\n", path.c_str());
  } else {
    Serial.println(F("mkdir: failed to create directory"));
  }
}

void handleTouch(const TokenizedLine &cmd) {
  if (cmd.count < 2) {
    Serial.println(F("Usage: touch <path>"));
    return;
  }

  String path = harixos::resolvePath(currentWorkingDirectory, cmd.tokens[1]);
  if (harixos::touch(path)) {
    Serial.printf("Touched: %s\n", path.c_str());
  } else {
    Serial.println(F("touch: failed"));
  }
}

void handleRm(const TokenizedLine &cmd) {
  if (cmd.count < 2) {
    Serial.println(F("Usage: rm <path>"));
    return;
  }

  bool recursive = false;
  String pathArg;
  if (cmd.tokens[1] == F("-r") || cmd.tokens[1] == F("-R")) {
    recursive = true;
    if (cmd.count < 3) {
      Serial.println(F("Usage: rm -r <path>"));
      return;
    }
    pathArg = cmd.tokens[2];
  } else {
    pathArg = cmd.tokens[1];
  }

  String path = harixos::resolvePath(currentWorkingDirectory, pathArg);
  if (!harixos::exists(path)) {
    Serial.println(F("rm: path not found"));
    return;
  }

  if (harixos::removePath(path)) {
    Serial.printf("Removed: %s%s\n", path.c_str(), recursive ? " (recursive)" : "");
  } else {
    Serial.println(F("rm: failed"));
  }
}

void handleCp(const TokenizedLine &cmd) {
  if (cmd.count < 3) {
    Serial.println(F("Usage: cp <source> <destination>"));
    return;
  }

  String source = harixos::resolvePath(currentWorkingDirectory, cmd.tokens[1]);
  String destination = harixos::resolvePath(currentWorkingDirectory, cmd.tokens[2]);
  if (harixos::copyFile(source, destination)) {
    Serial.printf("Copied %s -> %s\n", source.c_str(), destination.c_str());
  } else {
    Serial.println(F("cp: failed"));
  }
}

void handleMv(const TokenizedLine &cmd) {
  if (cmd.count < 3) {
    Serial.println(F("Usage: mv <source> <destination>"));
    return;
  }

  String source = harixos::resolvePath(currentWorkingDirectory, cmd.tokens[1]);
  String destination = harixos::resolvePath(currentWorkingDirectory, cmd.tokens[2]);
  if (harixos::movePath(source, destination)) {
    Serial.printf("Moved %s -> %s\n", source.c_str(), destination.c_str());
  } else {
    Serial.println(F("mv: failed"));
  }
}

void handleCat(const TokenizedLine &cmd) {
  if (cmd.count < 2) {
    Serial.println(F("Usage: cat <path>"));
    return;
  }

  String path = harixos::resolvePath(currentWorkingDirectory, cmd.tokens[1]);
  String content = harixos::readText(path);
  if (content.length() == 0 && !harixos::exists(path)) {
    Serial.println(F("cat: file not found"));
    return;
  }

  Serial.print(content);
  if (!content.endsWith("\n")) {
    Serial.println();
  }
}

void handleWrite(const TokenizedLine &cmd) {
  if (cmd.count < 3) {
    Serial.println(F("Usage: write <path> \"content\""));
    return;
  }

  String path = harixos::resolvePath(currentWorkingDirectory, cmd.tokens[1]);
  String content = cmd.tokens[2];
  if (harixos::writeText(path, content, false)) {
    Serial.printf("Wrote %u bytes to %s\n", content.length(), path.c_str());
  } else {
    Serial.println(F("write: failed"));
  }
}

void handleAppend(const TokenizedLine &cmd) {
  if (cmd.count < 3) {
    Serial.println(F("Usage: append <path> \"content\""));
    return;
  }

  String path = harixos::resolvePath(currentWorkingDirectory, cmd.tokens[1]);
  String content = cmd.tokens[2];
  if (harixos::writeText(path, content, true)) {
    Serial.printf("Appended %u bytes to %s\n", content.length(), path.c_str());
  } else {
    Serial.println(F("append: failed"));
  }
}

void handleNotepad(const TokenizedLine &cmd) {
  if (cmd.count < 2) {
    Serial.println(F("Usage: notepad <path>"));
    return;
  }

  String path = harixos::resolvePath(currentWorkingDirectory, cmd.tokens[1]);
  harixos::runNotepad(path);
}

void handleSettings(const TokenizedLine &cmd) {
  if (cmd.count < 2) {
    harixos::printSettings(shellSettings, Serial);
    Serial.println(F("Commands: settings show | settings banner on|off | settings timezone <tz> | settings update on|off | settings save | settings reload"));
    return;
  }

  String action = toLowerCopy(cmd.tokens[1]);
  if (action == F("show")) {
    harixos::printSettings(shellSettings, Serial);
  } else if (action == F("banner")) {
    if (cmd.count < 3) {
      Serial.println(F("Usage: settings banner on|off"));
      return;
    }
    String value = toLowerCopy(cmd.tokens[2]);
    if (value == F("on")) {
      shellSettings.bannerEnabled = true;
    } else if (value == F("off")) {
      shellSettings.bannerEnabled = false;
    } else {
      Serial.println(F("Use on or off."));
      return;
    }
    if (harixos::saveSettings(shellSettings)) {
      Serial.println(F("Settings saved."));
    } else {
      Serial.println(F("Failed to save settings."));
    }
  } else if (action == F("update")) {
    if (cmd.count < 3) {
      Serial.println(F("Usage: settings update on|off"));
      return;
    }
    String value = toLowerCopy(cmd.tokens[2]);
    if (value == F("on")) {
      shellSettings.autoUpdateCheck = true;
    } else if (value == F("off")) {
      shellSettings.autoUpdateCheck = false;
    } else {
      Serial.println(F("Use on or off."));
      return;
    }
    if (harixos::saveSettings(shellSettings)) {
      Serial.println(F("Settings saved. Auto update check updated."));
    } else {
      Serial.println(F("Failed to save settings."));
    }
  } else if (action == F("timezone") || action == F("tz")) {
    if (cmd.count < 3) {
      Serial.println(F("Usage: settings timezone <tz_string>"));
      Serial.println(F("Example: settings timezone UTC0 or PKT-5"));
      return;
    }
    shellSettings.timezone = cmd.tokens[2];
    setenv("TZ", shellSettings.timezone.c_str(), 1);
    tzset();
    if (harixos::saveSettings(shellSettings)) {
      Serial.println(F("Settings saved. Timezone updated."));
    } else {
      Serial.println(F("Failed to save settings."));
    }
  } else if (action == F("save")) {
    if (harixos::saveSettings(shellSettings)) {
      Serial.println(F("Settings saved."));
    } else {
      Serial.println(F("Failed to save settings."));
    }
  } else if (action == F("reload")) {
    shellSettings = harixos::loadSettings();
    Serial.println(F("Settings reloaded."));
  } else {
    Serial.println(F("Unknown settings action."));
  }
}

// --- Simple expression evaluator (shunting-yard + RPN) ---
bool isOp(char c) {
  return c == '+' || c == '-' || c == '*' || c == '/';
}

int prec(char op) {
  if (op == '+' || op == '-') return 1;
  if (op == '*' || op == '/') return 2;
  return 0;
}

double applyOp(double a, double b, char op) {
  switch (op) {
    case '+': return a + b;
    case '-': return a - b;
    case '*': return a * b;
    case '/': return b == 0 ? NAN : a / b;
  }
  return NAN;
}

double evalExpression(const String &expr, bool &ok) {
  const size_t MAXTOK = 128;
  double valStack[MAXTOK];
  char opStack[MAXTOK];
  int vTop = -1;
  int oTop = -1;

  size_t i = 0;
  size_t n = expr.length();
  while (i < n) {
    char c = expr[i];
    if (isspace((unsigned char)c)) { ++i; continue; }
    if (c == '(') {
      if (oTop + 1 >= (int)MAXTOK) { ok = false; return NAN; }
      opStack[++oTop] = c; ++i; continue;
    }
    if (c == ')') {
      while (oTop >= 0 && opStack[oTop] != '(') {
        if (vTop < 1) { ok = false; return NAN; }
        double b = valStack[vTop--];
        double a = valStack[vTop--];
        char op = opStack[oTop--];
        valStack[++vTop] = applyOp(a, b, op);
      }
      if (oTop >= 0 && opStack[oTop] == '(') --oTop;
      ++i; continue;
    }
    if (isOp(c)) {
      while (oTop >= 0 && isOp(opStack[oTop]) && prec(opStack[oTop]) >= prec(c)) {
        if (vTop < 1) { ok = false; return NAN; }
        double b = valStack[vTop--];
        double a = valStack[vTop--];
        char op = opStack[oTop--];
        valStack[++vTop] = applyOp(a, b, op);
      }
      opStack[++oTop] = c;
      ++i; continue;
    }
    // number
    if (isdigit((unsigned char)c) || c == '.') {
      String num;
      while (i < n && (isdigit((unsigned char)expr[i]) || expr[i] == '.')) {
        num += expr[i++];
      }
      double v = atof(num.c_str());
      valStack[++vTop] = v;
      continue;
    }
    // unknown char
    ok = false; return NAN;
  }

  while (oTop >= 0) {
    if (opStack[oTop] == '(' || opStack[oTop] == ')') { ok = false; return NAN; }
    if (vTop < 1) { ok = false; return NAN; }
    double b = valStack[vTop--];
    double a = valStack[vTop--];
    char op = opStack[oTop--];
    valStack[++vTop] = applyOp(a, b, op);
  }

  if (vTop != 0) { ok = false; return NAN; }
  ok = true;
  return valStack[vTop];
}

void handleCalc(const TokenizedLine &cmd) {
  if (cmd.count < 2) {
    Serial.println(F("Usage: calc <expression>  e.g. calc 1+2*(3-4)/5"));
    return;
  }
  String expr = cmd.tokens[1];
  bool ok = false;
  double res = evalExpression(expr, ok);
  if (!ok || isnan(res)) {
    Serial.println(F("Invalid expression."));
    return;
  }
  Serial.printf("= %.10g\n", res);
}

void handleWifiScan() {
  WiFi.mode(WIFI_STA);
  delay(50);

  Serial.println(F("Scanning..."));
  int networks = WiFi.scanNetworks(false, true);
  if (networks < 0) {
    Serial.println(F("Scan failed."));
    return;
  }

  if (networks == 0) {
    Serial.println(F("No networks found."));
    WiFi.scanDelete();
    return;
  }

  for (int index = 0; index < networks; ++index) {
    Serial.printf("%2d. %s\r\n", index + 1, WiFi.SSID(index).c_str());
    Serial.printf("    RSSI: %d dBm\r\n", WiFi.RSSI(index));
    Serial.printf("    CH: %d\r\n", WiFi.channel(index));
    Serial.printf("    Security: %s\r\n", WiFi.encryptionType(index) == ENC_TYPE_NONE ? "OPEN" : "SECURE");
  }
  WiFi.scanDelete();
}

void handleWifiConnect(const TokenizedLine &cmd) {
  if (cmd.count < 3) {
    Serial.println(F("Usage: wifi connect 'SSID' 'PASS'"));
    Serial.println(F("Example: wifi connect 'My WiFi' 'secret123'"));
    return;
  }

  String ssid = cmd.tokens[2];
  String password = cmd.count > 3 ? cmd.tokens[3] : String();

  Serial.print("Connecting to ");
  Serial.print(ssid);
  Serial.flush();
  delay(50);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000UL) {
    delay(200);
    Serial.print('.');
    if (httpServerRunning && httpServer) {
      httpServer->handleClient();
    }
    yield();
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    WiFi.setAutoReconnect(true); // Keep connection alive during this session
    Serial.println(F("Connected."));
    
    // Save credentials to settings
    shellSettings.wifiSSID = ssid;
    shellSettings.wifiPassword = password;
    harixos::saveSettings(shellSettings);
    Serial.println(F("WiFi credentials saved. Auto-connect enabled."));

    printWifiStatus();
    // Sync time via NTP now that we have internet
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  } else {
    Serial.printf("Connection failed: %s\r\n", wifiStatusToString(WiFi.status()).c_str());
  }
}

// Non-blocking line read with frequent yields (timeout 15s, checks HTTP every iteration)
String readLineBlocking() {
  String line;
  unsigned long start = millis();
  while (millis() - start < 15000UL) {
    while (Serial.available() > 0) {
      char c = static_cast<char>(Serial.read());
      if (c == '\r') continue;
      if (c == '\n') {
        return line;
      }
      if (isPrintable(static_cast<unsigned char>(c)) && line.length() < kMaxLineLength) {
        line += c;
      }
    }
    if (httpServerRunning && httpServer) {
      httpServer->handleClient();
    }
    delay(50);
    yield();
  }
  return String();
}

// Serve a requested LittleFS file and log requests
void handleHttpRequest() {
  if (!httpServer) return;
  String uri = httpServer->uri();
  String method = (httpServer->method() == HTTP_GET) ? "GET" : "OTHER";
  IPAddress remote = httpServer->client().remoteIP();
  Serial.printf("HTTP %s %s from %s\n", method.c_str(), uri.c_str(), remote.toString().c_str());

  String filePath;
  if (uri == "/") {
    filePath = httpServeFile;
  } else {
    String rel = uri;
    if (rel.startsWith("/")) rel = rel.substring(1);
    filePath = httpServeDir;
    if (!filePath.endsWith("/")) filePath += '/';
    filePath += rel;
  }

  filePath = harixos::normalizePath(filePath);
  if (!LittleFS.exists(filePath)) {
    httpServer->send(404, "text/plain", "Not found");
    return;
  }

  String contentType = "application/octet-stream";
  if (filePath.endsWith(".html") || filePath.endsWith(".htm")) contentType = "text/html";
  else if (filePath.endsWith(".css")) contentType = "text/css";
  else if (filePath.endsWith(".js")) contentType = "application/javascript";
  else if (filePath.endsWith(".png")) contentType = "image/png";
  else if (filePath.endsWith(".jpg") || filePath.endsWith(".jpeg")) contentType = "image/jpeg";
  else if (filePath.endsWith(".gif")) contentType = "image/gif";
  else if (filePath.endsWith(".txt")) contentType = "text/plain";

  File f = LittleFS.open(filePath, "r");
  if (!f) {
    httpServer->send(500, "text/plain", "Failed to open file");
    return;
  }
  httpServer->streamFile(f, contentType);
  f.close();
}

void handleServe(const TokenizedLine &cmd) {
  if (cmd.count < 2) {
    Serial.println(F("Usage: serve <file>|stop|status [port]"));
    return;
  }

  String action = toLowerCopy(cmd.tokens[1]);
  if (action == F("stop")) {
    handleHttpStop();
    return;
  }
  if (action == F("status")) {
    if (httpServerRunning && httpServer) {
      Serial.printf("HTTP serving %s on %s:%u\r\n", httpServeFile.c_str(), WiFi.localIP().toString().c_str(), httpServePort);
    } else {
      Serial.println(F("HTTP server not running."));
    }
    return;
  }

  String target = harixos::resolvePath(currentWorkingDirectory, cmd.tokens[1]);
  if (!harixos::exists(target)) {
    Serial.println(F("serve: file not found"));
    return;
  }
  if (harixos::isDirectory(target)) {
    Serial.println(F("serve: must be a file, not a directory"));
    return;
  }

  uint16_t port = 80;
  if (cmd.count >= 3) {
    port = static_cast<uint16_t>(cmd.tokens[2].toInt());
    if (port == 0) port = 80;
  }

  // Ensure WiFi connected; if not, prompt user for SSID/password
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("Not connected to WiFi. Enter SSID (blank to cancel):"));
    String ssid = readLineBlocking();
    if (ssid.length() == 0) {
      Serial.println(F("serve cancelled."));
      return;
    }
    Serial.println(F("Enter password (leave blank for open networks):"));
    String pass = readLineBlocking();
    Serial.print("Connecting to ");
    Serial.print(ssid);
    Serial.flush();
    delay(50);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());
    
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000UL) {
      delay(200);
      Serial.print('.');
      if (httpServer) {
        httpServer->handleClient();
      }
      yield();
    }
    Serial.println();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println(F("WiFi connection failed."));
      return;
    }
    Serial.println(F("Connected."));
  }

  // Stop existing server if running
  if (httpServer) {
    httpServer->stop();
    delete httpServer;
    httpServer = nullptr;
    httpServerRunning = false;
  }

  httpServeFile = target;
  httpServeDir = harixos::parentPath(target);
  httpServePort = port;

  httpServer = new ESP8266WebServer(port);
  httpServer->onNotFound([]() { handleHttpRequest(); });
  httpServer->begin();
  httpServerRunning = true;

  Serial.printf("Serving %s on %s:%u\r\n", httpServeFile.c_str(), WiFi.localIP().toString().c_str(), httpServePort);
}

void handleHttpStop() {
  if (httpServer) {
    httpServer->stop();
    delete httpServer;
    httpServer = nullptr;
  }
  httpServerRunning = false;
  Serial.println(F("HTTP server stopped."));
}

void handleRun(const TokenizedLine &cmd) {
  if (cmd.count < 2) {
    Serial.println(F("Usage: run <app_name>|<path> | run list | run install <name> | run uninstall <name>"));
    return;
  }

  String action = toLowerCopy(cmd.tokens[1]);

  if (action == F("list")) {
    harixos::api::AppManager::listApps(Serial);
    return;
  }

  if (action == F("install")) {
    if (cmd.count < 3) {
      Serial.println(F("Usage: run install <name>"));
      return;
    }
    Serial.println(F("Enter app content (type 'END' on a new line to finish):"));
    String content;
    while (true) {
      String line = readLineBlocking();
      if (line == "END") break;
      content += line + "\n";
    }
    harixos::api::ApiResult result = harixos::api::AppManager::installApp(cmd.tokens[2], content);
    Serial.println(result.message);
    return;
  }

  if (action == F("uninstall")) {
    if (cmd.count < 3) {
      Serial.println(F("Usage: run uninstall <name>"));
      return;
    }
    harixos::api::ApiResult result = harixos::api::AppManager::uninstallApp(cmd.tokens[2]);
    Serial.println(result.message);
    return;
  }

  // Default: run app by name or path
  String appName = cmd.tokens[1];
  harixos::api::ApiResult result = harixos::api::AppManager::runApp(appName, Serial);
  if (result.isError()) {
    Serial.println(result.message);
  }
}

/* Duplicate about() removed; single canonical implementation exists earlier. */

void handlePull(const TokenizedLine &cmd) {
  if (cmd.count < 3) {
    Serial.println(F("Usage: pull <url> <save_path>"));
    Serial.println(F("Example: pull http://example.com/app.hx /data/app.hx"));
    Serial.println();
    Serial.println(F("Supported protocols: http, https"));
    return;
  }

  String url = cmd.tokens[1];
  String savePath = cmd.tokens[2];

  // Check WiFi connection
  if (!harixos::HttpDownloader::isWiFiConnected()) {
    Serial.println(F("Not connected to WiFi."));
    Serial.println(F("Connect using: wifi connect <ssid> <password>"));
    Serial.println();
    
    // Offer WiFi scan
    Serial.println(F("Available networks:"));
    WiFi.mode(WIFI_STA);
    delay(100);
    int networks = WiFi.scanNetworks(false, true);
    if (networks > 0) {
      for (int i = 0; i < networks && i < 10; ++i) {
        Serial.printf("  %d. %s (RSSI: %d)\r\n", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i));
      }
      WiFi.scanDelete();
    }
    return;
  }

  // Download file
  bool success = harixos::HttpDownloader::downloadFile(url, savePath, Serial);
  
  if (success) {
    Serial.println(F("File download complete!"));
  } else {
    Serial.println(F("File download failed."));
  }
}

// Helper to check for updates (can be used manually or during boot)
void checkSystemUpdates(bool silent) {
  if (WiFi.status() != WL_CONNECTED) {
    if (!silent) Serial.println(F("Error: Please connect to WiFi first."));
    return;
  }

  if (!silent) Serial.println(F("Checking for updates..."));
  
  const char* updateUrl = "https://raw.githubusercontent.com/Haris16-code/HarixOS/refs/heads/main/updates/esp8266/update.json";
  
  // Use a block to ensure memory is released as soon as possible
  {
    WiFiClientSecure client;
    client.setInsecure();
    // Reduce buffer sizes to save memory on ESP-01
    client.setBufferSizes(1024, 512); 
    
    HTTPClient http;
    http.setTimeout(10000);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    
    // Add cache-busting timestamp to the URL to ensure we get a fresh copy from GitHub
    String freshUrl = String(updateUrl) + "?t=" + String(millis());
    
    if (http.begin(client, freshUrl)) {
      // Force no-cache headers
      http.addHeader("Cache-Control", "no-cache");
      http.addHeader("Pragma", "no-cache");
      
      int httpCode = http.GET();
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        http.end(); // Close connection early
        
        // Manual JSON Parsing (Version)
        int verPos = payload.indexOf("\"version\":");
        if (verPos != -1) {
          int start = payload.indexOf('"', verPos + 10);
          int end = payload.indexOf('"', start + 1);
          if (start != -1 && end != -1) {
            String newVersion = payload.substring(start + 1, end);
            
            if (newVersion != HARIXOS_VERSION_STRING) {
              Serial.println(F("\r\n[!] A new update is available!"));
              Serial.printf("Current version: %s\r\n", HARIXOS_VERSION_STRING);
              Serial.printf("Latest version:  %s\r\n", newVersion.c_str());
              
              // Manual JSON Parsing (Update Link)
              int linkPos = payload.indexOf("\"update_url\":");
              String dlLink = "";
              if (linkPos != -1) {
                int lStart = payload.indexOf('"', linkPos + 13);
                int lEnd = payload.indexOf('"', lStart + 1);
                if (lStart != -1 && lEnd != -1) {
                  dlLink = payload.substring(lStart + 1, lEnd);
                }
              }

              // Manual JSON Parsing (Changelog Array)
              int logsPos = payload.indexOf("\"changelog\":");
              if (logsPos != -1) {
                Serial.println(F("\r\nWhat's New:"));
                int arrayStart = payload.indexOf('[', logsPos);
                int arrayEnd = payload.indexOf(']', arrayStart);
                if (arrayStart != -1 && arrayEnd != -1) {
                  String logsArray = payload.substring(arrayStart + 1, arrayEnd);
                  
                  int lastPos = 0;
                  while (true) {
                    int logStart = logsArray.indexOf('"', lastPos);
                    if (logStart == -1) break;
                    int logEnd = logsArray.indexOf('"', logStart + 1);
                    if (logEnd == -1) break;
                    Serial.printf(" - %s\r\n", logsArray.substring(logStart + 1, logEnd).c_str());
                    lastPos = logEnd + 1;
                    yield();
                  }
                }
              }
              
              if (dlLink.length() > 0) {
                Serial.println();
                Serial.println(F("Download new update at:"));
                Serial.println(dlLink);
              } else {
                Serial.println(F("\r\nPlease visit GitHub to download the latest binary."));
              }
              Serial.println();
            } else {
              if (!silent) {
                Serial.printf("Current version: %s\r\n", HARIXOS_VERSION_STRING);
                Serial.println(F("HarixOS is up to date."));
              }
            }
          }
        }
        payload = String(); // Explicitly discard from RAM
      } else {
        if (!silent) Serial.printf("Error: Update check failed (HTTP %d)\r\n", httpCode);
      }
      http.end();
    } else {
      if (!silent) Serial.println(F("Error: Could not start HTTP request."));
    }
  } // WiFiClientSecure and HTTPClient destroyed here
}

void handleUpdate(const TokenizedLine &cmd) {
  if (cmd.count < 2 || toLowerCopy(cmd.tokens[1]) != "check") {
    Serial.println(F("Usage: update check"));
    return;
  }
  
  checkSystemUpdates(false);
}

void handleWifiDisconnect() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.println(F("WiFi disconnected and radio disabled."));
}

void handleWifiAp(const TokenizedLine &cmd) {
  if (cmd.count < 3) {
    Serial.println(F("Usage: wifi ap <ssid> [password] [channel]"));
    return;
  }

  String ssid = cmd.tokens[2];
  String password = cmd.count > 3 ? cmd.tokens[3] : String("12345678");
  uint8_t channel = cmd.count > 4 ? static_cast<uint8_t>(cmd.tokens[4].toInt()) : kDefaultApChannel;
  if (channel == 0) {
    channel = kDefaultApChannel;
  }

  WiFi.mode(WIFI_AP);
  bool started = WiFi.softAP(ssid.c_str(), password.c_str(), channel, false, 4);
  if (!started) {
    Serial.println(F("Failed to start access point."));
    return;
  }

  Serial.printf("AP started: %s\r\n", ssid.c_str());
  Serial.printf("  IP: %s\r\n", WiFi.softAPIP().toString().c_str());
  printWifiStatus();
}

void handleWifiMode(const TokenizedLine &cmd) {
  if (cmd.count < 3) {
    Serial.println(F("Usage: wifi mode off|sta|ap|staap"));
    return;
  }

  String mode = toLowerCopy(cmd.tokens[2]);
  if (mode == F("off")) {
    WiFi.mode(WIFI_OFF);
  } else if (mode == F("sta")) {
    WiFi.mode(WIFI_STA);
  } else if (mode == F("ap")) {
    WiFi.mode(WIFI_AP);
  } else if (mode == F("staap")) {
    WiFi.mode(WIFI_AP_STA);
  } else {
    Serial.println(F("Unknown WiFi mode."));
    return;
  }

  Serial.printf("WiFi mode set to %s\r\n", wifiModeToString(WiFi.getMode()).c_str());
}

void handleWifiIp() {
  Serial.printf("Station IP: %s\r\n", WiFi.localIP().toString().c_str());
  Serial.printf("Gateway: %s\r\n", WiFi.gatewayIP().toString().c_str());
  Serial.printf("DNS: %s\r\n", WiFi.dnsIP().toString().c_str());
  Serial.printf("Subnet: %s\r\n", WiFi.subnetMask().toString().c_str());
  Serial.printf("AP IP: %s\r\n", WiFi.softAPIP().toString().c_str());
}

void handleWifiMac() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  Serial.printf("Station MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  if (WiFi.softAPmacAddress(mac)) {
    Serial.printf("AP MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  }
}

void handleGpioList() {
  printGpioPins();
}

void handleGpioMode(const TokenizedLine &cmd) {
  if (cmd.count < 4) {
    Serial.println(F("Usage: gpio mode <pin> in|out|pullup"));
    return;
  }

  uint8_t pin;
  if (!parsePin(cmd.tokens[2], pin) || !isAvailableGpio(pin)) {
    Serial.println(F("Invalid or reserved GPIO for ESP8266."));
    return;
  }

  String mode = toLowerCopy(cmd.tokens[3]);
  if (mode == F("in") || mode == F("input")) {
    pinMode(pin, INPUT);
  } else if (mode == F("pullup") || mode == F("input_pullup")) {
    pinMode(pin, INPUT_PULLUP);
  } else if (mode == F("out") || mode == F("output")) {
    pinMode(pin, OUTPUT);
  } else {
    Serial.println(F("Unknown mode. Use in, out, or pullup."));
    return;
  }

  Serial.printf("GPIO%u mode set to %s\r\n", pin, mode.c_str());
}

void handleGpioRead(const TokenizedLine &cmd) {
  if (cmd.count < 3) {
    Serial.println(F("Usage: gpio read <pin>"));
    return;
  }

  uint8_t pin;
  if (!parsePin(cmd.tokens[2], pin) || !isAvailableGpio(pin)) {
    Serial.println(F("Invalid or reserved GPIO for ESP8266."));
    return;
  }

  Serial.printf("GPIO%u = %d\r\n", pin, digitalRead(pin));
}

void handleGpioWrite(const TokenizedLine &cmd) {
  if (cmd.count < 4) {
    Serial.println(F("Usage: gpio write <pin> on|off|toggle|0|1"));
    return;
  }

  uint8_t pin;
  if (!parsePin(cmd.tokens[2], pin) || !isAvailableGpio(pin)) {
    Serial.println(F("Invalid or reserved GPIO for ESP8266."));
    return;
  }

  String v = toLowerCopy(cmd.tokens[3]);
  if (isBootStrapPin(pin)) {
    int target = -1;
    if (v == F("on") || v == F("1")) {
      target = HIGH;
    } else if (v == F("off") || v == F("0")) {
      target = LOW;
    } else if (v == F("toggle")) {
      Serial.println(F("Blocked: toggle on boot strap pins can leave unsafe boot state."));
      return;
    }

    if (target != -1 && isUnsafeBootLevel(pin, target)) {
      Serial.println(F("Blocked: requested level is unsafe for ESP8266 boot straps."));
      return;
    }
  }

  pinMode(pin, OUTPUT);
  if (v == F("on") || v == F("1")) {
    digitalWrite(pin, HIGH);
    Serial.printf("GPIO%u -> ON\r\n", pin);
  } else if (v == F("off") || v == F("0")) {
    digitalWrite(pin, LOW);
    Serial.printf("GPIO%u -> OFF\r\n", pin);
  } else if (v == F("toggle")) {
    int cur = digitalRead(pin);
    digitalWrite(pin, cur ? LOW : HIGH);
    Serial.printf("GPIO%u -> %s\r\n", pin, digitalRead(pin) ? "ON" : "OFF");
  } else {
    Serial.println(F("Unknown value. Use on/off/toggle/0/1."));
    return;
  }
  Serial.println(F("Warning: keep boot strap pins in safe states before reboot."));
}

void handleGpioPulse(const TokenizedLine &cmd) {
  if (cmd.count < 4) {
    Serial.println(F("Usage: gpio pulse <pin> <count> [delay_ms]"));
    return;
  }

  uint8_t pin;
  if (!parsePin(cmd.tokens[2], pin) || !isAvailableGpio(pin)) {
    Serial.println(F("Invalid or reserved GPIO for ESP8266."));
    return;
  }

  int count = cmd.tokens[3].toInt();
  int delayMs = cmd.count > 4 ? cmd.tokens[4].toInt() : 250;
  if (count <= 0 || delayMs < 0) {
    Serial.println(F("Invalid pulse parameters."));
    return;
  }

  pinMode(pin, OUTPUT);
  for (int index = 0; index < count; ++index) {
    digitalWrite(pin, HIGH);
    delay(delayMs);
    digitalWrite(pin, LOW);
    delay(delayMs);
    yield();
  }

  if (isBootStrapPin(pin)) {
    // Ensure strap pins are left in a safe post-command state.
    if (pin == 0 || pin == 2) {
      digitalWrite(pin, HIGH);
    } else if (pin == 15) {
      digitalWrite(pin, LOW);
    }
  }

  Serial.printf("GPIO%u pulsed %d times\r\n", pin, count);
}

void handleI2cBegin(const TokenizedLine &cmd) {
  if (cmd.count < 4) {
    Serial.println(F("Usage: i2c begin <sda_pin> <scl_pin>"));
    Serial.println(F("Choose board-appropriate GPIO pins for software I2C."));
    return;
  }

  uint8_t sda;
  uint8_t scl;
  if (!parsePin(cmd.tokens[2], sda) || !parsePin(cmd.tokens[3], scl)) {
    Serial.println(F("Invalid I2C pins."));
    return;
  }

  Wire.begin(sda, scl);
  Serial.printf("I2C started on SDA GPIO%u, SCL GPIO%u\r\n", sda, scl);
}

void handleI2cScan() {
  Serial.println(F("Scanning I2C bus..."));
  int found = 0;
  for (uint8_t address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();
    if (error == 0) {
      Serial.printf("  Found device at 0x%02X\r\n", address);
      ++found;
    }
    delay(1);
    yield();
  }
  if (found == 0) {
    Serial.println(F("  No devices found."));
  }
}

void handleI2c(const TokenizedLine &cmd) {
  if (cmd.count < 2) {
    Serial.println(F("Usage: i2c begin <sda> <scl> | i2c scan"));
    return;
  }

  String action = toLowerCopy(cmd.tokens[1]);
  if (action == F("begin")) {
    handleI2cBegin(cmd);
  } else if (action == F("scan")) {
    handleI2cScan();
  } else {
    Serial.println(F("Unknown I2C action."));
  }
}

void handleWifi(const TokenizedLine &cmd) {
  if (cmd.count < 2) {
    printWifiStatus();
    return;
  }

  String action = toLowerCopy(cmd.tokens[1]);
  if (action == F("status")) {
    printWifiStatus();
  } else if (action == F("scan")) {
    handleWifiScan();
  } else if (action == F("connect")) {
    handleWifiConnect(cmd);
  } else if (action == F("disconnect")) {
    handleWifiDisconnect();
  } else if (action == F("ap")) {
    handleWifiAp(cmd);
  } else if (action == F("mode")) {
    handleWifiMode(cmd);
  } else if (action == F("ip")) {
    handleWifiIp();
  } else if (action == F("mac")) {
    handleWifiMac();
  } else {
    Serial.println(F("Unknown WiFi action."));
  }
}

void tryAutoWifi() {
  if (shellSettings.wifiSSID.length() == 0) {
    return;
  }

  Serial.printf("Auto-connect: Scanning for '%s'...\r\n", shellSettings.wifiSSID.c_str());
  WiFi.mode(WIFI_STA);
  int n = WiFi.scanNetworks();
  bool found = false;
  for (int i = 0; i < n; ++i) {
    if (WiFi.SSID(i) == shellSettings.wifiSSID) {
      found = true;
      break;
    }
  }
  WiFi.scanDelete();

  if (found) {
    Serial.print("Network found. Connecting");
    WiFi.begin(shellSettings.wifiSSID.c_str(), shellSettings.wifiPassword.c_str());
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000UL) {
      delay(500);
      Serial.print('.');
      yield();
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println(F("Auto-connected to WiFi."));
      configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    } else {
      Serial.println(F("Auto-connect failed."));
    }
  } else {
    Serial.println(F("Saved network not in range."));
  }
}

void handleGpio(const TokenizedLine &cmd) {
  if (cmd.count < 2) {
    handleGpioList();
    return;
  }

  String action = toLowerCopy(cmd.tokens[1]);
  if (action == F("list") || action == F("pins")) {
    handleGpioList();
  } else if (action == F("mode")) {
    handleGpioMode(cmd);
  } else if (action == F("read")) {
    handleGpioRead(cmd);
  } else if (action == F("write")) {
    handleGpioWrite(cmd);
  } else if (action == F("pulse")) {
    handleGpioPulse(cmd);
  } else {
    Serial.println(F("Unknown GPIO action."));
  }
}

void handleTime(const TokenizedLine &cmd) {
  if (cmd.count == 1) {
    time_t now = time(nullptr);
    if (now < 1000000000) {
      Serial.println(F("Time is not set. Use 'time set <HH:MM:SS> <YYYY-MM-DD>' or 'time sync'"));
      return;
    }
    struct tm *timeinfo = localtime(&now);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", timeinfo);
    Serial.printf("Current Time: %s\r\n", buf);
    return;
  }
  
  String action = toLowerCopy(cmd.tokens[1]);
  if (action == F("sync")) {
    if (cmd.count >= 3) {
      shellSettings.timezone = cmd.tokens[2];
      harixos::saveSettings(shellSettings);
      setenv("TZ", shellSettings.timezone.c_str(), 1);
      tzset();
      Serial.printf("Timezone updated to: %s\r\n", shellSettings.timezone.c_str());
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println(F("Syncing time via NTP..."));
      configTime(0, 0, "pool.ntp.org", "time.nist.gov");
      delay(500); // Give it a brief moment
      Serial.println(F("NTP sync requested. Check 'time' in a few seconds."));
    } else {
      Serial.println(F("Cannot sync time. WiFi is not connected."));
    }
  } else if (action == F("list-tz") || action == F("timezones")) {
    Serial.println(F("Common POSIX Timezones:"));
    Serial.println(F("  UTC:   UTC0"));
    Serial.println(F("  UK:    GMT0BST,M3.5.0/1,M10.5.0"));
    Serial.println(F("  CET:   CET-1CEST,M3.5.0,M10.5.0/3"));
    Serial.println(F("  EET:   EET-2EEST,M3.5.0/3,M10.5.0/4"));
    Serial.println(F("  EST:   EST5EDT,M3.2.0,M11.1.0"));
    Serial.println(F("  CST:   CST6CDT,M3.2.0,M11.1.0"));
    Serial.println(F("  MST:   MST7MDT,M3.2.0,M11.1.0"));
    Serial.println(F("  PST:   PST8PDT,M3.2.0,M11.1.0"));
    Serial.println(F("  India: IST-5:30"));
    Serial.println(F("  Japan: JST-9"));
    Serial.println(F("  AEST:  AEST-10AEDT,M10.1.0,M4.1.0/3"));
    Serial.println(F("For others, search online for 'POSIX TZ strings'"));
  } else if (action == F("set")) {
    if (cmd.count < 4) {
      Serial.println(F("Usage: time set <HH:MM:SS> <YYYY-MM-DD>"));
      return;
    }
    struct tm t = {0};
    int hh, mm, ss, YYYY, MM, DD;
    if (sscanf(cmd.tokens[2].c_str(), "%d:%d:%d", &hh, &mm, &ss) == 3 &&
        sscanf(cmd.tokens[3].c_str(), "%d-%d-%d", &YYYY, &MM, &DD) == 3) {
      t.tm_hour = hh;
      t.tm_min = mm;
      t.tm_sec = ss;
      t.tm_year = YYYY - 1900;
      t.tm_mon = MM - 1;
      t.tm_mday = DD;
      time_t now = mktime(&t);
      struct timeval tv = { .tv_sec = now, .tv_usec = 0 };
      settimeofday(&tv, nullptr);
      Serial.println(F("Time manually set."));
    } else {
      Serial.println(F("Invalid time format."));
    }
  } else {
    Serial.println(F("Unknown time action."));
  }
}

void handleSchedule(const TokenizedLine &cmd) {
  if (cmd.count < 2) {
    harixos::kernel::systemScheduler.listTasks(Serial);
    Serial.println(F("Usage: schedule add <HH:MM[:SS]> <command> | schedule add +<delay>[s|m|h] <command> | schedule remove <id> | schedule list"));
    return;
  }

  String action = toLowerCopy(cmd.tokens[1]);
  if (action == F("list")) {
    harixos::kernel::systemScheduler.listTasks(Serial);
  } else if (action == F("add")) {
    if (cmd.count < 4) {
      Serial.println(F("Usage: schedule add <time> <command>"));
      return;
    }

    String timeStr = cmd.tokens[2];
    String fullCommand;
    for (size_t i = 3; i < cmd.count; ++i) {
      if (i > 3) fullCommand += " ";
      fullCommand += cmd.tokens[i];
    }

    if (timeStr.startsWith("+")) {
      // Relative time: +5s, +1m, +2h
      unsigned long delayMs = 0;
      char unit = timeStr[timeStr.length() - 1];
      if (isdigit(unit)) {
        delayMs = timeStr.substring(1).toInt() * 1000;
      } else {
        long val = timeStr.substring(1, timeStr.length() - 1).toInt();
        if (unit == 's') delayMs = (unsigned long)val * 1000;
        else if (unit == 'm') delayMs = (unsigned long)val * 60000;
        else if (unit == 'h') delayMs = (unsigned long)val * 3600000;
        else {
          Serial.println(F("Invalid delay unit. Use s, m, or h."));
          return;
        }
      }
      int id = harixos::kernel::systemScheduler.addOnceTask(delayMs, fullCommand);
      if (id >= 0) Serial.printf("Task scheduled (once) with ID %d\r\n", id);
      else Serial.println(F("Scheduler full."));
    } else {
      // Absolute time: HH:MM:SS or HH:MM
      int hh, mm, ss = 0;
      int items = sscanf(timeStr.c_str(), "%d:%d:%d", &hh, &mm, &ss);
      if (items >= 2) {
        int id = harixos::kernel::systemScheduler.addDailyTask(hh, mm, ss, fullCommand);
        if (id >= 0) Serial.printf("Task scheduled (daily) with ID %d\r\n", id);
        else Serial.println(F("Scheduler full."));
      } else {
        Serial.println(F("Invalid time format. Use HH:MM[:SS] or +delay"));
      }
    }
  } else if (action == F("remove")) {
    if (cmd.count < 3) {
      Serial.println(F("Usage: schedule remove <id>"));
      return;
    }
    int id = cmd.tokens[2].toInt();
    if (harixos::kernel::systemScheduler.removeTask(id)) {
      Serial.println(F("Task removed."));
    } else {
      Serial.println(F("Task not found."));
    }
  } else {
    Serial.println(F("Unknown schedule action."));
  }
}

void handleHelp(const TokenizedLine &cmd) {
  if (cmd.count == 1) {
    Serial.println(F("HarixOS command shell"));
    Serial.println();
    Serial.println(F("System commands:"));
    Serial.println(F("  help                 Show all commands"));
    Serial.println(F("  about                Show version and features"));
    Serial.println(F("  info                 Show system, WiFi, and GPIO summary"));
    Serial.println(F("  chip                 Show chip and flash details"));
    Serial.println(F("  heap                 Show free heap"));
    Serial.println(F("  uptime               Show runtime"));
    Serial.println(F("  reboot               Reboot the ESP8266"));
    Serial.println(F("  adc                  Read A0"));
    Serial.println(F("  pull <url> <path>    Download file from internet"));
    Serial.println(F("  time                 Show current time & sync options"));
    Serial.println(F("  schedule             Manage background tasks"));
    Serial.println(F("  update check         Check for system updates"));
    Serial.println();
    Serial.println(F("Filesystem:"));
    Serial.println(F("  pwd                  Print current directory"));
    Serial.println(F("  cd <path>            Change directory"));
    Serial.println(F("  ls [-R] [path]       List files and folders"));
    Serial.println(F("  mkdir <path>         Create folders"));
    Serial.println(F("  touch <path>         Create empty files"));
    Serial.println(F("  rm [-r] <path>       Remove files or folders"));
    Serial.println(F("  cp <src> <dst>       Copy files"));
    Serial.println(F("  mv <src> <dst>       Move files"));
    Serial.println(F("  cat <path>           Print file contents"));
    Serial.println(F("  write <path> <txt>   Replace file contents"));
    Serial.println(F("  append <path> <txt>  Append to file"));
    Serial.println(F("  fs ...               Filesystem (LittleFS) tools"));
    Serial.println();
    Serial.println(F("Hardware & networking:"));
    Serial.println(F("  gpio ...             GPIO control and listing"));
    Serial.println(F("  wifi ...             WiFi connection and scanning"));
    Serial.println(F("  i2c ...              I2C bus tools"));
    Serial.println();
    Serial.println(F("Applications:"));
    Serial.println(F("  notepad <path>       Open text editor"));
    Serial.println(F("  settings ...         View or change shell settings"));
    Serial.println(F("  run ...              Install/list/run/uninstall .hx apps"));
    Serial.println(F("  calc <expr>          Evaluate arithmetic expressions"));
    Serial.println(F("  serve ...            HTTP file server tools"));
    Serial.println();
    Serial.println(F("Help topics:"));
    Serial.println(F("  help wifi|gpio|fs|serve|time|schedule|update  Show topic help"));
    return;
  }

  String topic = toLowerCopy(cmd.tokens[1]);
  if (topic == F("wifi")) {
    Serial.println(F("WiFi commands:"));
    Serial.println(F("  wifi status              Show current WiFi status"));
    Serial.println(F("  wifi scan                Scan available networks"));
    Serial.println(F("  wifi connect 'SSID' 'PW' Connect to network (saved to settings)"));
    Serial.println(F("  wifi disconnect          Disconnect WiFi"));
    Serial.println(F("  wifi ap 'SSID' 'PW'      Start access point"));
    Serial.println(F("  wifi mode off|sta|ap|staap Set WiFi mode"));
    Serial.println(F("  wifi ip                  Show IP configuration"));
    Serial.println(F("  wifi mac                 Show MAC addresses"));
    Serial.println();
    Serial.println(F("Note: Once connected via 'wifi connect', credentials are saved."));
    Serial.println(F("      On boot, HarixOS will auto-connect if the network is in range."));
  } else if (topic == F("gpio")) {
    Serial.println(F("GPIO commands:"));
    Serial.println(F("  gpio list"));
    Serial.println(F("  gpio mode <pin> in|out|pullup"));
    Serial.println(F("  gpio read <pin>"));
    Serial.println(F("  gpio write <pin> on|off|toggle|0|1"));
    Serial.println(F("  gpio pulse <pin> <count> [delay_ms]"));
  } else if (topic == F("fs")) {
    Serial.println(F("FS commands:"));
    Serial.println(F("  fs pwd"));
    Serial.println(F("  fs cd <path>"));
    Serial.println(F("  fs ls [-R] [path]"));
    Serial.println(F("  fs mkdir <path>"));
    Serial.println(F("  fs touch <path>"));
    Serial.println(F("  fs rm [-r] <path>"));
    Serial.println(F("  fs cp <src> <dst>"));
    Serial.println(F("  fs mv <src> <dst>"));
    Serial.println(F("  fs cat <path>"));
    Serial.println(F("  fs write <path> \"content\""));
    Serial.println(F("  fs append <path> \"content\""));
  } else if (topic == F("i2c")) {
    Serial.println(F("I2C commands:"));
    Serial.println(F("  i2c begin <sda_pin> <scl_pin>"));
    Serial.println(F("  i2c scan"));
  } else if (topic == F("serve")) {
    Serial.println(F("Serve commands:"));
    Serial.println(F("  serve <file> [port]"));
    Serial.println(F("  serve status"));
    Serial.println(F("  serve stop"));
    Serial.println(F("  Example: serve /index.html 80"));
  } else if (topic == F("time")) {
    Serial.println(F("Time commands:"));
    Serial.println(F("  time                     Show current time"));
    Serial.println(F("  time sync [timezone]     Sync NTP & optionally set timezone"));
    Serial.println(F("  time set <HH:MM:SS> <YYYY-MM-DD> Set time manually"));
    Serial.println(F("  time list-tz             Show common timezone codes"));
    Serial.println();
    Serial.println(F("Timezones:"));
    Serial.println(F("  Set via: settings timezone <POSIX_TZ_STRING>"));
    Serial.println(F("       or: time sync <POSIX_TZ_STRING>"));
    Serial.println(F("  Use 'time list-tz' for examples like IST-5:30"));
  } else if (topic == F("schedule")) {
    Serial.println(F("Scheduler commands:"));
    Serial.println(F("  schedule list            List all tasks"));
    Serial.println(F("  schedule remove <id>     Remove task by ID"));
    Serial.println(F("  schedule add <HH:MM:SS> <cmd>  Daily task"));
    Serial.println(F("  schedule add +<delay><unit> <cmd> One-off task"));
    Serial.println(F("  Units: s (sec), m (min), h (hour)"));
    Serial.println();
    Serial.println(F("Examples:"));
    Serial.println(F("  schedule add 14:30:00 'gpio 2 toggle'"));
    Serial.println(F("  schedule add +5s 'gpio 2 on'"));
    Serial.println(F("  schedule add +10s run test.hx"));
  } else if (topic == F("update")) {
    Serial.println(F("Update commands:"));
    Serial.println(F("  update check         Check for system updates via GitHub"));
    Serial.println();
    Serial.println(F("Example:"));
    Serial.println(F("  update check"));
  } else {
    Serial.println(F("Unknown help topic."));
  }
}

void executeCommand(const String &line) {
  TokenizedLine cmd = tokenize(line);
  if (cmd.count == 0) {
    return;
  }

  String command = toLowerCopy(cmd.tokens[0]);
  if (command == F("help") || command == F("?")) {
    handleHelp(cmd);
  } else if (command == F("about")) {
    handleAboutInfo();
  } else if (command == F("info")) {
    handleInfo();
  } else if (command == F("chip")) {
    handleChip();
  } else if (command == F("heap")) {
    handleHeap();
  } else if (command == F("uptime")) {
    handleUptime();
  } else if (command == F("reboot")) {
    handleReset();
  } else if (command == F("reset")) {
    Serial.println(F("Command renamed: use reboot"));
    handleReset();
  } else if (command == F("adc") || command == F("a0")) {
    handleAdc();
  } else if (command == F("pull")) {
    handlePull(cmd);
  } else if (command == F("pwd")) {
    handlePwd();
  } else if (command == F("cd")) {
    handleCd(cmd);
  } else if (command == F("ls")) {
    handleLs(cmd);
  } else if (command == F("mkdir")) {
    handleMkdir(cmd);
  } else if (command == F("touch")) {
    handleTouch(cmd);
  } else if (command == F("rm")) {
    handleRm(cmd);
  } else if (command == F("cp")) {
    handleCp(cmd);
  } else if (command == F("mv")) {
    handleMv(cmd);
  } else if (command == F("cat")) {
    handleCat(cmd);
  } else if (command == F("write")) {
    handleWrite(cmd);
  } else if (command == F("append")) {
    handleAppend(cmd);
  } else if (command == F("notepad")) {
    handleNotepad(cmd);
  } else if (command == F("settings")) {
    handleSettings(cmd);
  } else if (command == F("wifi")) {
    handleWifi(cmd);
  } else if (command == F("serve")) {
    handleServe(cmd);
  } else if (command == F("run")) {
    handleRun(cmd);
  } else if (command == F("gpio")) {
    handleGpio(cmd);
  } else if (command == F("fs")) {
    handleFs(cmd);
  } else if (command == F("calc")) {
    handleCalc(cmd);
  } else if (command == F("i2c")) {
    handleI2c(cmd);
  } else if (command == F("cls") || command == F("clear")) {
    for (uint8_t index = 0; index < 20; ++index) {
      Serial.println();
    }
  } else if (command == F("time")) {
    handleTime(cmd);
  } else if (command == F("schedule")) {
    handleSchedule(cmd);
  } else if (command == F("update")) {
    handleUpdate(cmd);
  } else {
    Serial.printf("Unknown command: %s\r\n", cmd.tokens[0].c_str());
    Serial.println(F("Type help for the command list."));
  }
}



void handleSerialInput() {
  while (Serial.available() > 0) {
    char c = static_cast<char>(Serial.read());

    if (c == '\r' || c == '\n') {
      if (inputLine.length() > 0) {
        Serial.println();
        String line = trimCopy(inputLine);
        inputLine = String();
        promptVisible = false;
        executeCommand(line);
      } else if (promptVisible) {
        Serial.println();
      }
      if (!promptVisible) {
        printPrompt();
      }
      continue;
    }

    if (c == '\b' || c == 127) {
      if (inputLine.length() > 0) {
        inputLine.remove(inputLine.length() - 1);
        Serial.print(F("\b \b"));
      }
      continue;
    }

    if (isPrintable(static_cast<unsigned char>(c)) && inputLine.length() < kMaxLineLength) {
      inputLine += c;
      Serial.write(c);
    }
  }
}

void printBanner() {
  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F(" HarixOS"));
  Serial.println(F(" Type help for commands"));
  Serial.println(F("========================================"));
  Serial.println();
}

void handleFs(const TokenizedLine &cmd) {
  if (cmd.count < 2) {
    handleLs(cmd);
    return;
  }
  String action = toLowerCopy(cmd.tokens[1]);
  if (action == F("pwd")) {
    handlePwd();
  } else if (action == F("cd")) {
    handleCd(cmd);
  } else if (action == F("ls") || action == F("list")) {
    handleLs(cmd);
  } else if (action == F("mkdir")) {
    handleMkdir(cmd);
  } else if (action == F("touch")) {
    handleTouch(cmd);
  } else if (action == F("rm") || action == F("remove")) {
    handleRm(cmd);
  } else if (action == F("cp")) {
    handleCp(cmd);
  } else if (action == F("mv")) {
    handleMv(cmd);
  } else if (action == F("cat") || action == F("read")) {
    handleCat(cmd);
  } else if (action == F("write")) {
    handleWrite(cmd);
  } else if (action == F("append")) {
    handleAppend(cmd);
  } else if (action == F("notepad")) {
    handleNotepad(cmd);
  } else if (action == F("settings")) {
    handleSettings(cmd);
  } else {
    Serial.println(F("Unknown fs action."));
  }
}

}  // namespace


extern "C" {
  #include "user_interface.h"
}

// Force the RF (radio) to be completely off at boot to prevent brownouts on ESP-01
RF_PRE_INIT() {
  system_phy_set_powerup_option(3);
}

void setup() {
  
  Serial.begin(115200);
  Serial.setTimeout(25);
  delay(500);

  Serial.println();
  Serial.println(F("***********************************"));
  Serial.println(F("*** SYSTEM HARDWARE BOOTING ***"));
  Serial.println(F("***********************************"));
  Serial.flush();
  delay(100);

  // Disable automatic WiFi during boot to prevent crash loops
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  delay(1);

  Wire.begin();
  if (LittleFS.begin()) {
    Serial.println(F("LittleFS mounted."));
  } else {
    Serial.println(F("LittleFS mount failed."));
  }

  shellSettings = harixos::loadSettings();
  setenv("TZ", shellSettings.timezone.c_str(), 1);
  tzset();

  if (shellSettings.bannerEnabled) {
    printBanner();
    printSystemInfo();
  } else {
    Serial.println(F("HarixOS ready."));
  }
  Serial.println();
  
  tryAutoWifi();
  harixos::kernel::CpuHandler::init();

  // If update check is enabled, check now and show results.
  if (shellSettings.autoUpdateCheck) {
    delay(500); // Small delay to let network settle
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println(F("System: Running auto-update check..."));
      checkSystemUpdates(false); // Show results even if up to date
    }
  }
  
  printPrompt();
  
  // Ensure the serial buffer is completely sent before starting the main loop
  Serial.flush();
  delay(100);
}

void loop() {
  handleSerialInput();
  if (httpServerRunning && httpServer) {
    httpServer->handleClient();
  }
  harixos::kernel::systemScheduler.update();
  
  // Use safe yield to process background tasks and feed WDT
  harixos::kernel::CpuHandler::yieldSafely();
}
