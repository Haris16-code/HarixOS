#include "cpu_handler.h"

extern "C" {
  #include "user_interface.h"
}

namespace harixos {
namespace kernel {

void CpuHandler::init() {
  // CPU frequency is handled by the default SDK boot process (80MHz)
  Serial.println(F("CPU: Initialized at default 80 MHz clock."));
}

void CpuHandler::optimizeTaskHandling() {
  // Process pending background tasks efficiently and feed the hardware watchdog
  ESP.wdtFeed();
  yield();
}

void CpuHandler::yieldSafely() {
  // Safe yield that ensures the WDT is fed and WiFi/TCP stacks get processed
  // Adding delay(1) forces a context switch to the SDK background tasks
  ESP.wdtFeed();
  delay(1); 
}

} // namespace kernel
} // namespace harixos
