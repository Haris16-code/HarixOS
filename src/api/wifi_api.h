#ifndef HARIXOS_WIFI_API_H
#define HARIXOS_WIFI_API_H

#include "api_types.h"
#include <ESP8266WiFi.h>

namespace harixos {
namespace api {

// ===== WiFi API =====
class WiFiAPI {
public:
  // Scan for available networks
  static ApiResult scan(Stream &output);
  
  // Connect to WiFi network
  static ApiResult connect(const String &ssid, const String &password, uint32_t timeoutMs = 10000);
  
  // Disconnect from WiFi
  static ApiResult disconnect();
  
  // Get WiFi status
  static ApiResult status(Stream &output);
  
  // Get IP information
  static ApiResult getIP(Stream &output);
  
  // Start access point
  static ApiResult startAP(const String &ssid, const String &password, uint8_t channel = 1);
  
  // Stop access point
  static ApiResult stopAP();
  
private:
  static String wifiStatusToString(int status);
};

}  // namespace api
}  // namespace harixos

#endif
