# SMP Wind

A Skyrim mod that adds wind physics to HDT-SMP.

## Dependencies

- [CommonLibSSE-NG (alandtse/CommonLibVR, `ng`)](https://github.com/alandtse/CommonLibVR) — git submodule, pinned to the commit FSMP uses
- [Faster HDT-SMP (DaymareOn/hdtSMP64)](https://github.com/DaymareOn/hdtSMP64) at runtime — plugin interface 2.x
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444) at runtime
- vcpkg (manifest mode) for CommonLib's dependencies and Bullet headers

The plugin is version independent (Address Library) and works on SE 1.5.97, AE 1.6.x and 1.7.x.

## Build instructions

Requires Visual Studio 2022 (MSVC v143), CMake ≥ 3.22 and vcpkg with `VCPKG_ROOT` set.

```
git clone --recurse-submodules <this repo>
cmake --preset release
cmake --build --preset release
```

The DLL lands in `build/Release/SMP Wind.dll`. Pass `-DOUTPUT_DIR="<mod folder>"` at configure time to copy it to `<mod folder>/SKSE/Plugins` after each build.

Without a Windows machine: push to a GitHub fork; `.github/workflows/build.yml` builds on `windows-2022` and uploads `SKSE/Plugins/SMP Wind.dll` as a workflow artifact.
