#ifndef HARIXOS_API_TYPES_H
#define HARIXOS_API_TYPES_H

#include <Arduino.h>

namespace harixos {
namespace api {

// ===== API Status Codes =====
enum ApiStatus {
  API_OK = 0,
  API_ERROR = -1,
  API_INVALID_PIN = -2,
  API_INVALID_GPIO_MODE = -3,
  API_FILE_NOT_FOUND = -4,
  API_PERMISSION_DENIED = -5,
  API_INVALID_ARGUMENT = -6,
  API_WIFI_NOT_CONNECTED = -7,
  API_WIFI_ALREADY_CONNECTED = -8,
  API_WIFI_CONNECTION_FAILED = -9,
  API_TIMEOUT = -10,
  API_OUT_OF_MEMORY = -11,
  API_NOT_IMPLEMENTED = -12,
};

// ===== API Result Structure =====
struct ApiResult {
  ApiStatus status;
  String message;
  
  ApiResult(ApiStatus s = API_OK, const String &msg = "") 
    : status(s), message(msg) {}
  
  bool isSuccess() const { return status == API_OK; }
  bool isError() const { return status != API_OK; }
};

// ===== GPIO Result (returns pin value) =====
struct GpioResult : ApiResult {
  int value;
  
  GpioResult(ApiStatus s = API_OK, int v = 0, const String &msg = "") 
    : ApiResult(s, msg), value(v) {}
};

// ===== File Result (returns content) =====
struct FileResult : ApiResult {
  String content;
  
  FileResult(ApiStatus s = API_OK, const String &c = "", const String &msg = "") 
    : ApiResult(s, msg), content(c) {}
};

// ===== System Info Structure =====
struct SystemInfo {
  uint32_t chipId;
  String coreVersion;
  String sdkVersion;
  uint8_t cpuFreq;
  uint32_t heapFree;
  uint32_t uptime;
  uint32_t flashSize;
  String resetReason;
};

}  // namespace api
}  // namespace harixos

#endif
