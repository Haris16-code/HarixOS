#ifndef HARIXOS_GPIO_API_H
#define HARIXOS_GPIO_API_H

#include "api_types.h"

namespace harixos {
namespace api {

// ===== GPIO API =====
class GpioAPI {
public:
  // Set GPIO mode (INPUT, OUTPUT, INPUT_PULLUP)
  static ApiResult setMode(uint8_t pin, uint8_t mode);
  
  // Write GPIO value (HIGH/LOW)
  static ApiResult write(uint8_t pin, uint8_t value);
  
  // Read GPIO value
  static GpioResult read(uint8_t pin);
  
  // Toggle GPIO
  static ApiResult toggle(uint8_t pin);
  
  // Pulse GPIO (on-off repetition)
  static ApiResult pulse(uint8_t pin, int count, int delayMs = 250);
  
  // Get all available pins for this board
  static void listAvailablePins(Stream &output);
  
private:
  // Validate if pin is available on this board
  static bool isAvailablePin(uint8_t pin);
  
  // Check if pin is a boot-critical pin
  static bool isBootStrapPin(uint8_t pin);
};

}  // namespace api
}  // namespace harixos

#endif
