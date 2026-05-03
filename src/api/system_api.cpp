#include "system_api.h"

namespace harixos {
namespace api {

SystemInfo SystemAPI::getInfo() {
  SystemInfo info;
  info.chipId = ESP.getChipId();
  info.coreVersion = ESP.getCoreVersion();
  info.sdkVersion = ESP.getSdkVersion();
  info.cpuFreq = ESP.getCpuFreqMHz();
  info.heapFree = ESP.getFreeHeap();
  info.uptime = millis();
  info.flashSize = ESP.getFlashChipRealSize();
  info.resetReason = ESP.getResetReason();
  return info;
}

ApiResult SystemAPI::printInfo(Stream &output) {
  SystemInfo info = getInfo();
  
  output.println(F("System Info:"));
  output.printf("  Chip ID: 0x%08X\r\n", info.chipId);
  output.printf("  Core: %s\r\n", info.coreVersion.c_str());
  output.printf("  SDK: %s\r\n", info.sdkVersion.c_str());
  output.printf("  CPU: %u MHz\r\n", info.cpuFreq);
  output.printf("  Heap Free: %u bytes\r\n", info.heapFree);
  output.printf("  Uptime: %u ms\r\n", info.uptime);
  output.printf("  Flash: %u bytes\r\n", info.flashSize);
  output.printf("  Reset Reason: %s\r\n", info.resetReason.c_str());
  
  return ApiResult(API_OK, "System info retrieved");
}

uint32_t SystemAPI::getHeapFree() {
  return ESP.getFreeHeap();
}

uint32_t SystemAPI::getUptime() {
  return millis();
}

uint32_t SystemAPI::getChipID() {
  return ESP.getChipId();
}

void SystemAPI::reboot() {
  Serial.println(F("Rebooting..."));
  Serial.flush();
  delay(100);
  ESP.restart();
}

void SystemAPI::delay(uint32_t ms) {
  ::delay(ms);
}

void SystemAPI::yield() {
  ::yield();
}

}  // namespace api
}  // namespace harixos
