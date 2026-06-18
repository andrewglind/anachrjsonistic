# Anachrjsonistic

The anachronistic JSON parser - written in [Haxe](https://haxe.org) and transpiled to C++98 standard C++ via [Hatchet](https://github.com/andrewglind/hatchet). It exposes a [Simpleson](https://github.com/gregjesl/simpleson)-like API, lightweight (header-only), and can be used with Visual C++ 6.0 projects.

## Build

```bash
hatchet --src json/Json.hx --header-only json --out . --force
```

## Tested platforms/compilers

- [x] Windows 98 with Visual C++ 6.0
- [x] Windows 11 with Visual Studio 2022
- [x] Windows 11 with Visual Studio 2026

## License

This project is licensed under the MIT License — see the [LICENSE](LICENSE) file for details.