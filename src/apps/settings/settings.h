#pragma once

#include <Arduino.h>

namespace harixos {

struct AppSettings {
  bool bannerEnabled = true;
  String timezone = "UTC0";
  String wifiSSID = "";
  String wifiPassword = "";
  bool autoUpdateCheck = true;
};

AppSettings loadSettings();
bool saveSettings(const AppSettings &settings);
void printSettings(const AppSettings &settings, Print &out);

}  // namespace harixos
