#ifndef HARIXOS_HTTP_DOWNLOADER_H
#define HARIXOS_HTTP_DOWNLOADER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

namespace harixos {

// HTTP/HTTPS file downloader utility
// Supports both HTTP and HTTPS protocols
// Automatically checks WiFi connection before download
class HttpDownloader {
public:
  // Download file from URL to LittleFS path
  // Supports both http:// and https:// URLs
  // Returns true on success, false on failure
  static bool downloadFile(const String &url, const String &savePath, Stream &output = Serial);
  
  // Check if WiFi is connected
  static bool isWiFiConnected();
  
  // Get file size from HTTP HEAD request (optional)
  // Returns 0 if unable to determine size
  static uint32_t getFileSize(const String &url);
  
  // Get LittleFS storage info
  // Returns free bytes available, sets totalBytes and usedBytes
  static uint32_t getStorageInfo(uint32_t &totalBytes, uint32_t &usedBytes);
  
private:
  // Helper to create directory if needed
  static bool ensureDirectory(const String &path);
  
  // Helper to parse URL into components
  // Returns: true if valid, false if invalid
  static bool parseUrl(const String &url, String &protocol, String &host, uint16_t &port, String &path);
  
  // Internal downloader supporting HTTP and HTTPS
  // Uses appropriate client based on protocol
  static bool performDownload(const String &protocol, const String &host, uint16_t port, 
                              const String &path, const String &savePath, Stream &output);
};

}  // namespace harixos

#endif
