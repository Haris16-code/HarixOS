#include "filesystem.h"

namespace harixos {
namespace {

bool isAbsolute(const String &path) {
  return path.startsWith("/");
}

String trimTrailingSlash(const String &path) {
  if (path.length() <= 1) {
    return String("/");
  }

  String result = path;
  while (result.length() > 1 && result.endsWith("/")) {
    result.remove(result.length() - 1);
  }
  return result;
}

bool ensureDirectoryChain(const String &path) {
  if (path.length() == 0 || path == "/") {
    return true;
  }

  String current = "/";
  size_t start = 1;
  while (start <= path.length()) {
    int slash = path.indexOf('/', start);
    String segment = slash == -1 ? path.substring(start) : path.substring(start, slash);
    if (segment.length() > 0) {
      if (current != "/") {
        current += '/';
      }
      current += segment;
      if (!LittleFS.exists(current)) {
        if (!LittleFS.mkdir(current)) {
          return false;
        }
      }
    }

    if (slash == -1) {
      break;
    }
    start = static_cast<size_t>(slash + 1);
  }

  return true;
}

String joinPath(const String &left, const String &right) {
  if (right.length() == 0) {
    return normalizePath(left);
  }
  if (isAbsolute(right)) {
    return normalizePath(right);
  }

  String base = normalizePath(left);
  if (!base.endsWith("/")) {
    base += '/';
  }
  base += right;
  return normalizePath(base);
}

bool fileIsDirectory(const String &path) {
  if (!LittleFS.exists(path)) {
    return false;
  }

  File file = LittleFS.open(path, "r");
  if (!file) {
    return false;
  }

  return file.isDirectory();
}

}  // namespace

String normalizePath(const String &path) {
  if (path.length() == 0) {
    return String("/");
  }

  String result;
  result.reserve(path.length());
  bool previousWasSlash = false;

  for (size_t index = 0; index < path.length(); ++index) {
    char c = path[index];
    if (c == '\\') {
      c = '/';
    }
    if (c == '/') {
      if (previousWasSlash) {
        continue;
      }
      previousWasSlash = true;
    } else {
      previousWasSlash = false;
    }
    result += c;
  }

  if (result.length() == 0) {
    result = "/";
  }
  return trimTrailingSlash(result);
}

String resolvePath(const String &cwd, const String &input) {
  if (input.length() == 0 || input == ".") {
    return normalizePath(cwd);
  }
  if (isAbsolute(input)) {
    return normalizePath(input);
  }
  return joinPath(cwd, input);
}

String parentPath(const String &path) {
  String normalized = normalizePath(path);
  if (normalized == "/") {
    return String("/");
  }

  int slash = normalized.lastIndexOf('/');
  if (slash <= 0) {
    return String("/");
  }

  return normalized.substring(0, slash);
}

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif

String fileName(const String &path) {
  String normalized = normalizePath(path);
  const char *slash = strrchr(normalized.c_str(), '/');
  if (!slash) {
    return normalized;
  }

  return String(slash + 1);
}

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

bool exists(const String &path) {
  return LittleFS.exists(normalizePath(path));
}

bool isDirectory(const String &path) {
  return fileIsDirectory(normalizePath(path));
}

bool makeDirectory(const String &path) {
  String normalized = normalizePath(path);
  return ensureDirectoryChain(normalized);
}

bool touch(const String &path) {
  String normalized = normalizePath(path);
  if (LittleFS.exists(normalized)) {
    return true;
  }

  String parent = parentPath(normalized);
  if (!ensureDirectoryChain(parent)) {
    return false;
  }

  File file = LittleFS.open(normalized, "w");
  if (!file) {
    return false;
  }
  file.close();
  return true;
}

bool writeText(const String &path, const String &content, bool append) {
  String normalized = normalizePath(path);
  String parent = parentPath(normalized);
  if (!ensureDirectoryChain(parent)) {
    return false;
  }

  File file = LittleFS.open(normalized, append ? "a" : "w");
  if (!file) {
    return false;
  }

  file.print(content);
  file.close();
  return true;
}

String readText(const String &path) {
  String normalized = normalizePath(path);
  File file = LittleFS.open(normalized, "r");
  if (!file) {
    return String();
  }

  String content = file.readString();
  file.close();
  return content;
}

bool copyFile(const String &sourcePath, const String &destinationPath) {
  String source = normalizePath(sourcePath);
  String destination = normalizePath(destinationPath);

  File input = LittleFS.open(source, "r");
  if (!input) {
    return false;
  }

  String parent = parentPath(destination);
  if (!ensureDirectoryChain(parent)) {
    input.close();
    return false;
  }

  File output = LittleFS.open(destination, "w");
  if (!output) {
    input.close();
    return false;
  }

  while (input.available()) {
    output.write(input.read());
  }

  input.close();
  output.close();
  return true;
}

bool movePath(const String &sourcePath, const String &destinationPath) {
  if (!copyFile(sourcePath, destinationPath)) {
    return false;
  }
  return removePath(sourcePath);
}

bool removePath(const String &path) {
  String normalized = normalizePath(path);
  if (normalized == "/") {
    return false;
  }

  if (!LittleFS.exists(normalized)) {
    return true;
  }

  if (!fileIsDirectory(normalized)) {
    return LittleFS.remove(normalized);
  }

  Dir dir = LittleFS.openDir(normalized);
  while (dir.next()) {
    String childName = dir.fileName();
    if (childName.length() == 0) {
      continue;
    }

    String childPath;
    // dir.fileName() may return either absolute or relative names depending on platform.
    if (childName.startsWith(normalized)) {
      childPath = childName;
    } else if (childName.startsWith("/")) {
      childPath = childName;
    } else {
      // build full path under the directory we are iterating
      childPath = normalized;
      if (!childPath.endsWith("/")) childPath += '/';
      childPath += childName;
    }

    if (!removePath(childPath)) {
      return false;
    }
  }

  return LittleFS.rmdir(normalized);
}

void listDirectory(const String &path, Print &out, bool recursive, uint8_t depth) {
  String normalized = normalizePath(path);
  if (!LittleFS.exists(normalized)) {
    out.printf("Path not found: %s\r\n", normalized.c_str());
    return;
  }

  if (!fileIsDirectory(normalized)) {
    out.printf("%s\r\n", normalized.c_str());
    return;
  }

  Dir dir = LittleFS.openDir(normalized);
  while (dir.next()) {
    String entry = dir.fileName();
    File file = LittleFS.open(entry, "r");
    bool entryIsDirectory = file && file.isDirectory();
    size_t size = file ? file.size() : 0;
    if (file) {
      file.close();
    }

    for (uint8_t index = 0; index < depth; ++index) {
      out.print("  ");
    }
    out.printf("%s%s  %u bytes\r\n", entry.c_str(), entryIsDirectory ? "/" : "", static_cast<unsigned>(size));

    if (recursive && entryIsDirectory) {
      listDirectory(entry, out, true, static_cast<uint8_t>(depth + 1));
    }
  }
}

}  // namespace harixos
