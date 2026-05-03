#include "wifi_api.h"
#include <ESP8266WiFi.h>

namespace harixos {
namespace api {

String WiFiAPI::wifiStatusToString(int status) {
  wl_status_t wlStatus = static_cast<wl_status_t>(status);
  switch (wlStatus) {
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

ApiResult WiFiAPI::scan(Stream &output) {
  WiFi.mode(WIFI_STA);
  delay(50);
  
  output.println(F("WiFi scan:"));
  int networks = WiFi.scanNetworks(false, true);
  
  if (networks < 0) {
    return ApiResult(API_ERROR, "Scan failed");
  }
  
  if (networks == 0) {
    output.println(F("  No networks found"));
    WiFi.scanDelete();
    return ApiResult(API_OK, "Scan complete: 0 networks");
  }
  
  for (int i = 0; i < networks; ++i) {
    output.printf("  %d. %s\r\n", i + 1, WiFi.SSID(i).c_str());
    output.printf("     RSSI: %d dBm\r\n", WiFi.RSSI(i));
    output.printf("     CH: %d\r\n", WiFi.channel(i));
    output.printf("     Security: %s\r\n", WiFi.encryptionType(i) == ENC_TYPE_NONE ? "OPEN" : "SECURE");
  }
  
  WiFi.scanDelete();
  return ApiResult(API_OK, "Scan complete: " + String(networks) + " networks");
}

ApiResult WiFiAPI::connect(const String &ssid, const String &password, uint32_t timeoutMs) {
  if (ssid.length() == 0) {
    return ApiResult(API_INVALID_ARGUMENT, "SSID cannot be empty");
  }
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    delay(200);
    yield();
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    return ApiResult(API_OK, "Connected to " + ssid);
  } else {
    return ApiResult(API_WIFI_CONNECTION_FAILED, "Failed: " + wifiStatusToString(WiFi.status()));
  }
}

ApiResult WiFiAPI::disconnect() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  return ApiResult(API_OK, "WiFi disconnected");
}

ApiResult WiFiAPI::status(Stream &output) {
  output.println(F("WiFi Status:"));
  output.printf("  Mode: %s\r\n", (WiFi.getMode() == WIFI_OFF) ? "OFF" : 
                                   (WiFi.getMode() == WIFI_STA) ? "STA" :
                                   (WiFi.getMode() == WIFI_AP) ? "AP" : "AP+STA");
  output.printf("  Status: %s\r\n", wifiStatusToString(WiFi.status()).c_str());
  output.printf("  Station SSID: %s\r\n", WiFi.SSID().c_str());
  output.printf("  Station IP: %s\r\n", WiFi.localIP().toString().c_str());
  output.printf("  RSSI: %d dBm\r\n", WiFi.isConnected() ? WiFi.RSSI() : 0);
  
  return ApiResult(API_OK, "Status retrieved");
}

ApiResult WiFiAPI::getIP(Stream &output) {
  output.println(F("Network Info:"));
  output.printf("  IP: %s\r\n", WiFi.localIP().toString().c_str());
  output.printf("  Gateway: %s\r\n", WiFi.gatewayIP().toString().c_str());
  output.printf("  Subnet: %s\r\n", WiFi.subnetMask().toString().c_str());
  output.printf("  DNS: %s\r\n", WiFi.dnsIP().toString().c_str());
  
  return ApiResult(API_OK, "IP info retrieved");
}

ApiResult WiFiAPI::startAP(const String &ssid, const String &password, uint8_t channel) {
  if (ssid.length() == 0) {
    return ApiResult(API_INVALID_ARGUMENT, "SSID cannot be empty");
  }
  
  if (channel < 1 || channel > 13) {
    return ApiResult(API_INVALID_ARGUMENT, "Channel must be 1-13");
  }
  
  WiFi.mode(WIFI_AP);
  bool started = WiFi.softAP(ssid.c_str(), password.c_str(), channel, false, 4);
  
  if (!started) {
    return ApiResult(API_ERROR, "Failed to start AP");
  }
  
  return ApiResult(API_OK, "AP started: " + ssid);
}

ApiResult WiFiAPI::stopAP() {
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  return ApiResult(API_OK, "AP stopped");
}

}  // namespace api
}  // namespace harixos
