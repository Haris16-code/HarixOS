#ifndef HARIXOS_CPU_HANDLER_H
#define HARIXOS_CPU_HANDLER_H

#include <Arduino.h>

namespace harixos {
namespace kernel {

class CpuHandler {
public:
  // Initialize CPU optimizations
  static void init();
  
  // Process pending background tasks efficiently
  static void optimizeTaskHandling();
  
  // A safe yield to prevent Watchdog resets during heavy operations
  static void yieldSafely();
};

} // namespace kernel
} // namespace harixos

#endif
