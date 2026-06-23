# Anachrjsonistic

The anachronistic JSON parser - written in [Haxe](https://haxe.org) and transpiled to C++98 standard C++ via [Hatchet](https://github.com/andrewglind/hatchet). It exposes a [Simpleson](https://github.com/gregjesl/simpleson)-like API, lightweight (header-only), and can be used with Visual C++ 6.0 projects.

## Build

```bash
hatchet --src json/Json.hx --header-only json --out . --force
```

## Usage

Include the generated header and parse a document with `json::parse`. Indexing a
`json::jobject` with `[]` returns a `json::proxy`, which converts implicitly to
`std::string`, `int`, `float`, `bool`, a nested `json::jobject`, or a
`std::vector<json::jobject>` — so you rarely need to spell out a conversion.

> **Note:** scalars (numbers and booleans) are stored as their original text.
> `toInt()` / `toFloat()` / `toBool()` parse that text on read, and an absent or
> wrong-typed key yields a safe zero value (`0`, `0.0`, `false`, `""`) rather
> than throwing.

### Reading scalar values

```cpp
#include "json.h"

json::jobject config = json::parse("{ \"width\": 800, \"enabled\": true, \"title\": \"example\" }");

int         width   = config["width"];    // 800
bool        enabled = config["enabled"];  // true
std::string title   = config["title"];    // "example"
```

### Optional values with fallbacks

Because a missing key returns a default-constructed value, it is convenient to
wrap reads in small helpers that fall back when a key is absent:

```cpp
bool OptionalBool(json::jobject& obj, const std::string& key, bool fallback) {
    return obj.hasKey(key) ? (bool) obj[key] : fallback;
}

int OptionalInt(json::jobject& obj, const std::string& key, int fallback) {
    return obj.hasKey(key) ? (int) obj[key] : fallback;
}

std::string OptionalString(json::jobject& obj, const std::string& key, const std::string& fallback) {
    return obj.hasKey(key) ? (std::string) obj[key] : fallback;
}
```

### Nested objects

A proxy can be assigned straight to a `json::jobject`, or chained with `[]` to
reach deeper into the document:

```cpp
json::jobject config = json::parse(text);

// Read the nested object, then pull values from it...
json::jobject display = config["display"];
int rate = OptionalInt(display, "refresh_rate", 60);

// ...or chain straight through.
int size = config["buffer"]["size"];
```

### Arrays of objects

A proxy over a JSON array converts to `std::vector<json::jobject>`, ready to
iterate:

```cpp
json::jobject config = json::parse(text);
if (config.hasKey("items")) {
    std::vector<json::jobject> items = config["items"];
    for (std::vector<json::jobject>::const_iterator it = items.begin(); it != items.end(); ++it) {
        json::jobject item = *it;
        std::string name = item["name"];
        bool active = OptionalBool(item, "active", true);
        // ...
    }
}
```

### Reading an object as a string map

When every value under an object is a scalar string, `toMap()` collapses it into
a `std::map<std::string, std::string>` in one call:

```cpp
json::jobject entries = config["entries"];
std::map<std::string, std::string> files = entries.toMap();
```

### Building and inspecting values

```cpp
json::jobject obj;                       // a fresh, empty object
obj.setString("name", "example");
obj.hasKey("name");                      // true

// Guard against missing/wrong-typed nodes before reading.
json::proxy node = config["section"];
if (node.isObject()) {
    json::jobject section = node;
    // ...
}
```

## Tests

The test suite lives in [`tests/`](tests/) and uses [doctest](https://github.com/doctest/doctest).
The library itself stays C++98/VC6-clean, but the tests build under a modern standard

With CMake:

```bash
cmake -B build && cmake --build build && ctest --test-dir build --output-on-failure
```

CI runs the suite on Linux (GCC and Clang), macOS (Clang), and Windows (MSVC)
via GitHub Actions.

## Tested platforms/compilers

- [x] Windows 98 with Visual C++ 6.0
- [x] Windows 11 with Visual Studio 2022
- [x] Windows 11 with Visual Studio 2026

## License

This project is licensed under the MIT License — see the [LICENSE](LICENSE) file for details.