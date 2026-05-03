#ifndef HARIXOS_SYSTEM_API_H
#define HARIXOS_SYSTEM_API_H

#include "api_types.h"

namespace harixos {
namespace api {

// ===== System API =====
class SystemAPI {
public:
  // Get system information
  static SystemInfo getInfo();
  
  // Print system info
  static ApiResult printInfo(Stream &output);
  
  // Get free heap
  static uint32_t getHeapFree();
  
  // Get uptime in milliseconds
  static uint32_t getUptime();
  
  // Get chip ID
  static uint32_t getChipID();
  
  // Reboot the device
  static void reboot();
  
  // Delay (blocking)
  static void delay(uint32_t ms);
  
  // Yield to scheduler
  static void yield();
};

}  // namespace api
}  // namespace harixos

#endif
