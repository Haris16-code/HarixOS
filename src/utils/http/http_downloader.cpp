#include <ESP8266WiFi.h>
#include "http_downloader.h"
#include <LittleFS.h>
#include "../../kernel/filesystem/filesystem.h"
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

namespace harixos {

bool HttpDownloader::isWiFiConnected() {
  return (WiFi.status() == WL_CONNECTED);
}

bool HttpDownloader::ensureDirectory(const String &path) {
  String parent = harixos::parentPath(path);
  if (parent.length() == 0 || parent == "/") return true;
  return harixos::makeDirectory(parent);
}

bool HttpDownloader::parseUrl(const String &url, String &protocol, String &host, uint16_t &port, String &path) {
  protocol = ""; host = ""; port = 0; path = "/";
  int pos = url.indexOf("://");
  if (pos == -1) return false;
  protocol = url.substring(0, pos);
  int start = pos + 3;
  int slash = url.indexOf('/', start);
  String hostPort;
  if (slash == -1) {
    hostPort = url.substring(start);
  } else {
    hostPort = url.substring(start, slash);
    path = url.substring(slash);
  }
  int colon = hostPort.indexOf(':');
  if (colon == -1) {
    host = hostPort;
    if (protocol == "http") port = 80;
    else if (protocol == "https") port = 443;
    else return false;
  } else {
    host = hostPort.substring(0, colon);
    port = static_cast<uint16_t>(hostPort.substring(colon + 1).toInt());
  }
  return true;
}

uint32_t HttpDownloader::getFileSize(const String &url) {
  String proto, host, path;
  uint16_t port;
  if (!parseUrl(url, proto, host, port, path)) return 0;

  if (!isWiFiConnected()) return 0;

  // Timeout protection: skip if HEAD request takes too long
  unsigned long startTime = millis();
  const unsigned long headTimeout = 5000;  // 5 second timeout for HEAD request

  if (proto == "https") {
    WiFiClientSecure *client = new WiFiClientSecure();
    client->setInsecure();
    client->setTimeout(3000);  // 3 second socket timeout
    HTTPClient *http = new HTTPClient();
    http->setTimeout(3000);
    http->setUserAgent("HarixOS/1.0 (ESP8266)");
    http->setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    String full = String("https://") + host + path;
    if (!http->begin(*client, full)) { delete http; delete client; return 0; }
    
    int code = http->sendRequest("HEAD");
    if (code <= 0) { http->end(); delete http; delete client; return 0; }
    
    if (millis() - startTime > headTimeout) {
      http->end();
      delete http; delete client;
      return 0;  // Timeout, skip size check
    }
    
    int64_t len = http->getSize();
    http->end();
    delete http; delete client;
    yield();
    return (len > 0) ? (uint32_t)len : 0;
  } else {
    WiFiClient *client = new WiFiClient();
    client->setTimeout(3000);
    HTTPClient *http = new HTTPClient();
    http->setTimeout(3000);
    http->setUserAgent("HarixOS/1.0 (ESP8266)");
    http->setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    String full = String("http://") + host + path;
    if (!http->begin(*client, full)) { delete http; delete client; return 0; }
    
    int code = http->sendRequest("HEAD");
    if (code <= 0) { http->end(); delete http; delete client; return 0; }
    
    if (millis() - startTime > headTimeout) {
      http->end();
      delete http; delete client;
      return 0;  // Timeout, skip size check
    }
    
    int64_t len = http->getSize();
    http->end();
    delete http; delete client;
    yield();
    return (len > 0) ? (uint32_t)len : 0;
  }
}

// Helper: get LittleFS info (returns bytes available or 0 on error)
uint32_t HttpDownloader::getStorageInfo(uint32_t &totalBytes, uint32_t &usedBytes) {
  FSInfo info;
  if (!LittleFS.info(info)) {
    return 0;
  }
  totalBytes = info.totalBytes;
  usedBytes = info.usedBytes;
  return info.totalBytes - info.usedBytes;  // free bytes
}

bool HttpDownloader::performDownload(const String &protocol, const String &host, uint16_t port,
                                     const String &path, const String &savePath, Stream &output) {
  String url = protocol + "://" + host + path;
  
  if (!ensureDirectory(savePath)) {
    output.println(F("Failed to create directory for save path."));
    return false;
  }

  if (!LittleFS.begin()) {
    output.println(F("LittleFS not mounted."));
    return false;
  }

  output.printf("Starting download: %s -> %s\r\n", url.c_str(), savePath.c_str());
  yield();

  File fh = LittleFS.open(savePath, "w");
  if (!fh) {
    output.println(F("Failed to open destination file for writing."));
    return false;
  }

  bool ok = false;
  
  if (protocol == "https") {
    WiFiClientSecure *client = new WiFiClientSecure();
    client->setInsecure();
    client->setTimeout(10000);  // 10 second socket timeout
    HTTPClient *http = new HTTPClient();
    http->setTimeout(10000);
    http->setUserAgent("HarixOS/1.0 (ESP8266)");
    http->setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    
    if (!http->begin(*client, url)) {
      output.println(F("HTTP begin failed."));
      delete http; delete client;
      fh.close();
      LittleFS.remove(savePath);
      return false;
    }
    
    int code = http->GET();
    if (code == HTTP_CODE_OK || (code >= 200 && code < 300)) {
      int len = http->getSize();
      
      uint32_t totalBytes = 0, usedBytes = 0;
      uint32_t freeBytes = getStorageInfo(totalBytes, usedBytes);
      
      if (len > 0) {
        output.printf("File size: %d bytes. Storage free: %u bytes.\r\n", len, freeBytes);
        uint32_t requiredSpace = len + (len / 10);
        if (requiredSpace > freeBytes) {
          output.println(F("ERROR: Insufficient storage!"));
          http->end(); delete http; delete client;
          fh.close();
          LittleFS.remove(savePath);
          return false;
        }
      } else {
        output.println(F("Warning: Could not determine file size (may be chunked)."));
        output.printf("Storage free: %u bytes. Proceeding...\r\n", freeBytes);
      }
      
      int bytesWritten = http->writeToStream(&fh);
      if (bytesWritten > 0 && (len == -1 || bytesWritten == len)) {
        ok = true;
        output.printf("Downloaded: %d bytes\r\n", bytesWritten);
      } else {
        output.printf("Download interrupted or failed. Bytes written: %d\r\n", bytesWritten);
      }
    } else {
      output.printf("HTTP GET failed: %d\r\n", code);
    }
    http->end();
    delete http; delete client;
  } else {
    // HTTP (not HTTPS)
    WiFiClient *client = new WiFiClient();
    client->setTimeout(10000);
    HTTPClient *http = new HTTPClient();
    http->setTimeout(10000);
    http->setUserAgent("HarixOS/1.0 (ESP8266)");
    http->setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    
    if (!http->begin(*client, url)) {
      output.println(F("HTTP begin failed."));
      delete http; delete client;
      fh.close();
      LittleFS.remove(savePath);
      return false;
    }
    
    int code = http->GET();
    if (code == HTTP_CODE_OK || (code >= 200 && code < 300)) {
      int len = http->getSize();
      
      uint32_t totalBytes = 0, usedBytes = 0;
      uint32_t freeBytes = getStorageInfo(totalBytes, usedBytes);
      
      if (len > 0) {
        output.printf("File size: %d bytes. Storage free: %u bytes.\r\n", len, freeBytes);
        uint32_t requiredSpace = len + (len / 10);
        if (requiredSpace > freeBytes) {
          output.println(F("ERROR: Insufficient storage!"));
          http->end(); delete http; delete client;
          fh.close();
          LittleFS.remove(savePath);
          return false;
        }
      } else {
        output.println(F("Warning: Could not determine file size (may be chunked)."));
        output.printf("Storage free: %u bytes. Proceeding...\r\n", freeBytes);
      }
      
      int bytesWritten = http->writeToStream(&fh);
      if (bytesWritten > 0 && (len == -1 || bytesWritten == len)) {
        ok = true;
        output.printf("Downloaded: %d bytes\r\n", bytesWritten);
      } else {
        output.printf("Download interrupted or failed. Bytes written: %d\r\n", bytesWritten);
      }
    } else {
      output.printf("HTTP GET failed: %d\r\n", code);
    }
    http->end();
    delete http; delete client;
  }

  fh.close();
  
  if (ok) {
    output.println(F("Download finished successfully!"));
  } else {
    output.println(F("Download failed."));
    // Clean up partial file
    LittleFS.remove(savePath);
  }
  
  yield();
  return ok;
}

bool HttpDownloader::downloadFile(const String &url, const String &savePath, Stream &output) {
  if (!isWiFiConnected()) {
    output.println(F("WiFi not connected - cannot download."));
    return false;
  }

  String protocol, host, path;
  uint16_t port;
  if (!parseUrl(url, protocol, host, port, path)) {
    output.println(F("Invalid URL."));
    return false;
  }

  return performDownload(protocol, host, port, path, savePath, output);
}

} // namespace harixos
