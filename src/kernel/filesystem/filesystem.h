#pragma once

#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>

namespace harixos {

String normalizePath(const String &path);
String resolvePath(const String &cwd, const String &input);
String parentPath(const String &path);
String fileName(const String &path);
bool makeDirectory(const String &path);
bool touch(const String &path);
bool exists(const String &path);
bool isDirectory(const String &path);
bool writeText(const String &path, const String &content, bool append = false);
String readText(const String &path);
bool copyFile(const String &sourcePath, const String &destinationPath);
bool movePath(const String &sourcePath, const String &destinationPath);
bool removePath(const String &path);
void listDirectory(const String &path, Print &out, bool recursive = false, uint8_t depth = 0);

}  // namespace harixos
