#include "notepad.h"

#include <FS.h>

#include "../../kernel/filesystem/filesystem.h"

namespace harixos {
namespace {

String readEditorLine() {
  String line;
  while (true) {
    while (Serial.available() == 0) {
      yield();
    }

    char c = static_cast<char>(Serial.read());
    if (c == '\r' || c == '\n') {
      Serial.println();
      break;
    }

    if (c == '\b' || c == 127) {
      if (line.length() > 0) {
        line.remove(line.length() - 1);
        Serial.print(F("\b \b"));
      }
      continue;
    }

    if (isPrintable(static_cast<unsigned char>(c))) {
      line += c;
      Serial.write(c);
    }
  }
  return line;
}

void printNotepadHelp() {
  Serial.println(F("Commands:"));
  Serial.println(F("  :help      Show commands"));
  Serial.println(F("  :show      Print current buffer"));
  Serial.println(F("  :clear     Clear buffer"));
  Serial.println(F("  :save      Save and stay in editor"));
  Serial.println(F("  :wq        Save and quit"));
  Serial.println(F("  :q         Quit without saving"));
}

}  // namespace

void runNotepad(const String &path) {
  String targetPath = normalizePath(path);
  String buffer = readText(targetPath);
  bool dirty = false;

  Serial.println();
  Serial.printf("HarixOS Notepad: %s\n", targetPath.c_str());
  Serial.println(F("Enter text line by line. Commands start with ':'."));
  Serial.println(F("Type :help for editor commands."));
  if (buffer.length() > 0) {
    Serial.printf("Loaded %u bytes.\n", static_cast<unsigned>(buffer.length()));
  }

  while (true) {
    Serial.print(F("notepad> "));
    String line = readEditorLine();

    if (line.startsWith(":")) {
      String command = line.substring(1);
      command.toLowerCase();

      if (command == F("help")) {
        printNotepadHelp();
      } else if (command == F("show")) {
        Serial.println(F("--- Buffer ---"));
        Serial.print(buffer);
        if (!buffer.endsWith("\n")) {
          Serial.println();
        }
        Serial.println(F("--------------"));
      } else if (command == F("clear")) {
        buffer = String();
        dirty = true;
        Serial.println(F("Buffer cleared."));
      } else if (command == F("save")) {
        if (writeText(targetPath, buffer, false)) {
          dirty = false;
          Serial.println(F("Saved."));
        } else {
          Serial.println(F("Save failed."));
        }
      } else if (command == F("wq")) {
        if (writeText(targetPath, buffer, false)) {
          Serial.println(F("Saved."));
        } else {
          Serial.println(F("Save failed."));
        }
        return;
      } else if (command == F("q")) {
        if (dirty) {
          Serial.println(F("Unsaved changes discarded."));
        }
        return;
      } else {
        Serial.println(F("Unknown editor command. Type :help."));
      }
      continue;
    }

    buffer += line;
    buffer += '\n';
    dirty = true;
  }
}

}  // namespace harixos
