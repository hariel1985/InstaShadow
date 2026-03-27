# InstaShadow

Dual-stage mastering compressor plugin (VST3/AU/LV2) inspired by the Shadow Hills Mastering Compressor, built with JUCE.

## Features

- **Optical Compressor** — Port-Hamiltonian T4B opto-cell model with physically accurate two-stage release and CdS memory effect
- **Discrete VCA Compressor** — Feed-forward Class-A topology with 7 ratio settings (1.2:1 to Flood), 6 attack/release presets, and Dual release mode
- **Output Transformer** — 3 switchable transformer types (Nickel/Iron/Steel) with frequency-dependent saturation and 4x oversampling
- **Sidechain HPF** — Variable 20-500 Hz high-pass filter to prevent bass-induced pumping
- **Stereo Link** — Linked or dual-mono operation
- **Independent bypass** — Each section can be bypassed separately
- **GR Metering** — Dedicated optical and discrete gain reduction meters
- **State save/restore** — All parameters persist with DAW session

## Signal Flow

```
Input → SC HPF → Optical Comp (T4B) → VCA Comp → Transformer → Output
```

## Build

Requires [JUCE](https://github.com/juce-framework/JUCE) cloned at `../JUCE` relative to this project.

```bash
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

## License

GPL-3.0
