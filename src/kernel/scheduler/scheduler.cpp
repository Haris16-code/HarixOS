#include "scheduler.h"
#include <time.h>
#include "../../api/script_engine.h"
#include "../filesystem/filesystem.h"

namespace harixos {
namespace kernel {

Scheduler systemScheduler;

Scheduler::Scheduler() : taskCount(0), nextId(1), lastExecutedSecond(-1), lastExecutedMinute(-1), lastExecutedHour(-1) {}

int Scheduler::addDailyTask(int hour, int minute, int second, const String &command) {
  if (taskCount >= MAX_TASKS) {
    return -1;
  }
  tasks[taskCount].id = nextId++;
  tasks[taskCount].type = TASK_DAILY;
  tasks[taskCount].hour = hour;
  tasks[taskCount].minute = minute;
  tasks[taskCount].second = second;
  tasks[taskCount].command = command;
  taskCount++;
  return tasks[taskCount - 1].id;
}

int Scheduler::addOnceTask(unsigned long delayMs, const String &command) {
  if (taskCount >= MAX_TASKS) {
    return -1;
  }
  tasks[taskCount].id = nextId++;
  tasks[taskCount].type = TASK_ONCE;
  tasks[taskCount].executeAtMillis = millis() + delayMs;
  tasks[taskCount].command = command;
  taskCount++;
  return tasks[taskCount - 1].id;
}

bool Scheduler::removeTask(int id) {
  for (int i = 0; i < taskCount; ++i) {
    if (tasks[i].id == id) {
      for (int j = i; j < taskCount - 1; ++j) {
        tasks[j] = tasks[j + 1];
      }
      taskCount--;
      return true;
    }
  }
  return false;
}

void Scheduler::listTasks(Print &out) {
  if (taskCount == 0) {
    out.println(F("No scheduled tasks."));
    return;
  }
  out.println(F("Scheduled Tasks:"));
  out.println(F("ID | Type  | Time     | Command"));
  out.println(F("-----------------------------------"));
  for (int i = 0; i < taskCount; ++i) {
    if (tasks[i].type == TASK_DAILY) {
      out.printf("%2d | DAILY | %02d:%02d:%02d | %s\r\n", tasks[i].id, tasks[i].hour, tasks[i].minute, tasks[i].second, tasks[i].command.c_str());
    } else {
      unsigned long remaining = (tasks[i].executeAtMillis > millis()) ? (tasks[i].executeAtMillis - millis()) / 1000 : 0;
      out.printf("%2d | ONCE  | In %lus  | %s\r\n", tasks[i].id, remaining, tasks[i].command.c_str());
    }
  }
}

void Scheduler::update() {
  time_t now = time(nullptr);
  struct tm *timeinfo = (now >= 1000000000) ? localtime(&now) : nullptr;
  
  if (timeinfo && timeinfo->tm_hour == lastExecutedHour && timeinfo->tm_min == lastExecutedMinute && timeinfo->tm_sec == lastExecutedSecond) {
    // Already checked for this exact second
  } else {
    if (timeinfo) {
      lastExecutedHour = timeinfo->tm_hour;
      lastExecutedMinute = timeinfo->tm_min;
      lastExecutedSecond = timeinfo->tm_sec;
    }
    
    // Check Daily Tasks
    if (timeinfo) {
      for (int i = 0; i < taskCount; ++i) {
        if (tasks[i].type == TASK_DAILY && tasks[i].hour == timeinfo->tm_hour && tasks[i].minute == timeinfo->tm_min && tasks[i].second == timeinfo->tm_sec) {
          Serial.printf("\r\n[Scheduler] Executing daily task #%d: %s\r\n", tasks[i].id, tasks[i].command.c_str());
          harixos::api::ScriptEngine::executeCommand(tasks[i].command, Serial);
          Serial.print(F("\r\nHarixOS> "));
        }
      }
    }
  }
  
  // Check Once Tasks (millis based)
  for (int i = 0; i < taskCount; ++i) {
    if (tasks[i].type == TASK_ONCE && millis() >= tasks[i].executeAtMillis) {
      Serial.printf("\r\n[Scheduler] Executing one-off task #%d: %s\r\n", tasks[i].id, tasks[i].command.c_str());
      harixos::api::ScriptEngine::executeCommand(tasks[i].command, Serial);
      Serial.print(F("\r\nHarixOS> "));
      removeTask(tasks[i].id);
      i--; // Adjust index since we removed an item
    }
  }
}

} // namespace kernel
} // namespace harixos
