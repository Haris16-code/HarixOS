#include "settings.h"

#include "../../kernel/filesystem/filesystem.h"

namespace harixos {
namespace {

const char *kSettingsPath = "/harixos/settings.cfg";

}  // namespace

AppSettings loadSettings() {
  AppSettings settings;
  if (!exists(kSettingsPath)) {
    return settings;
  }

  Serial.println(F("Loading system settings..."));
  String content = readText(kSettingsPath);
  int bannerPos = content.indexOf("banner=");
  if (bannerPos >= 0) {
    int endLine = content.indexOf('\n', bannerPos);
    String bannerVal = endLine == -1 ? content.substring(bannerPos + 7) : content.substring(bannerPos + 7, endLine);
    bannerVal.trim();
    if (bannerVal.equalsIgnoreCase("off")) {
      settings.bannerEnabled = false;
    }
  }

  int tzPos = content.indexOf("timezone=");
  if (tzPos >= 0) {
    int endLine = content.indexOf('\n', tzPos);
    String tzVal = endLine == -1 ? content.substring(tzPos + 9) : content.substring(tzPos + 9, endLine);
    tzVal.trim();
    if (tzVal.length() > 0) {
      settings.timezone = tzVal;
    }
  }



  int ssidPos = content.indexOf("wifiSSID=");
  if (ssidPos >= 0) {
    int endLine = content.indexOf('\n', ssidPos);
    settings.wifiSSID = endLine == -1 ? content.substring(ssidPos + 9) : content.substring(ssidPos + 9, endLine);
    settings.wifiSSID.trim();
  }

  int passPos = content.indexOf("wifiPassword=");
  if (passPos >= 0) {
    int endLine = content.indexOf('\n', passPos);
    settings.wifiPassword = endLine == -1 ? content.substring(passPos + 13) : content.substring(passPos + 13, endLine);
    settings.wifiPassword.trim();
  }
  
  int autoUpPos = content.indexOf("auto_update=");
  if (autoUpPos >= 0) {
    int endLine = content.indexOf('\n', autoUpPos);
    String autoUpVal = endLine == -1 ? content.substring(autoUpPos + 12) : content.substring(autoUpPos + 12, endLine);
    autoUpVal.trim();
    if (autoUpVal.equalsIgnoreCase("on")) {
      settings.autoUpdateCheck = true;
    } else if (autoUpVal.equalsIgnoreCase("off")) {
      settings.autoUpdateCheck = false;
    }
  }

  return settings;
}

bool saveSettings(const AppSettings &settings) {
  String content = String("banner=") + (settings.bannerEnabled ? "on" : "off") + "\n";
  content += String("timezone=") + settings.timezone + "\n";
  content += String("wifiSSID=") + settings.wifiSSID + "\n";
  content += String("wifiPassword=") + settings.wifiPassword + "\n";
  content += String("auto_update=") + (settings.autoUpdateCheck ? "on" : "off") + "\n";
  bool ok = writeText(kSettingsPath, content, false);
  if (ok) {
    Serial.println(F("System settings saved successfully."));
  }
  return ok;
}

void printSettings(const AppSettings &settings, Print &out) {
  out.println(F("Settings"));
  out.printf("  Boot banner: %s\n", settings.bannerEnabled ? "ON" : "OFF");
  out.printf("  Timezone: %s\r\n", settings.timezone.c_str());
  out.printf("  WiFi SSID: %s\r\n", settings.wifiSSID.length() > 0 ? settings.wifiSSID.c_str() : "(not set)");
  out.printf("  Auto Update Check: %s\n", settings.autoUpdateCheck ? "ON" : "OFF");
  out.printf("  Config file: %s\n", kSettingsPath);
}

}  // namespace harixos
