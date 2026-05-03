#ifndef HARIXOS_SCHEDULER_H
#define HARIXOS_SCHEDULER_H

#include <Arduino.h>

namespace harixos {
namespace kernel {

enum TaskType {
  TASK_DAILY,
  TASK_ONCE
};

struct ScheduledTask {
  int id;
  TaskType type;
  
  // For TASK_DAILY
  int hour;
  int minute;
  int second;
  
  // For TASK_ONCE
  unsigned long executeAtMillis;
  
  String command; // Can be a raw command or 'run <script.hx>'
};

class Scheduler {
public:
  Scheduler();
  
  // Add a daily task at HH:MM:SS
  int addDailyTask(int hour, int minute, int second, const String &command);
  
  // Add a one-off task after delay in milliseconds
  int addOnceTask(unsigned long delayMs, const String &command);
  
  // Remove task by ID
  bool removeTask(int id);
  
  // List all tasks to output
  void listTasks(Print &out);
  
  // Check and execute tasks. Should be called in loop()
  void update();
  
private:
  static const int MAX_TASKS = 20;
  ScheduledTask tasks[MAX_TASKS];
  int taskCount;
  int nextId;
  int lastExecutedSecond;
  int lastExecutedMinute;
  int lastExecutedHour;
};

// Global scheduler instance
extern Scheduler systemScheduler;

} // namespace kernel
} // namespace harixos

#endif // HARIXOS_SCHEDULER_H
