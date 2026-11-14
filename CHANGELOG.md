# Changelog

## 1.2.0 — unreleased

### Fixed
- **Presets and projects are recalled correctly** ([#1](https://github.com/hariel1985/InstaShadow/issues/1)). The editor used to keep its own copy of the settings and wrote it back over a state the host had just restored, so a preset or project loaded with the plugin window open reverted within a fraction of a second.
- The meters return to rest when the host stops processing, instead of freezing on the last value.

### Added
- **Every control is a host parameter**: automatable, shown in generic editors, and saved and recalled by the host's own presets. The plugin's Bypass is also the host's bypass.
- **9 factory presets**, also offered to the host as programs (REAPER's preset menu, AU factory presets).
- **User presets**: *Save* in the header writes a preset file. Previous/next and the preset menu step through factory and user presets, and a mark shows when the current preset has been edited.
- **Deleting user presets** from the preset menu, after a confirmation. The file goes to the Trash. Factory presets can't be deleted.
- **Three switchable themes**: Graphite & Brass, Mastering Console and Ivory Rack, chosen from the colour swatches in the header. The choice is saved with the plugin, and new instances start with the last one used.
- **A/B compare** in the Mastering Console theme.

### Changed
- **New editor**, 1000×540 and resizable (700×378 to 2000×1080) with a fixed aspect ratio.
- **The VU meters were redrawn**: the scale at the top, the needle pivot hidden behind a cover plate, and a 4:3 face. The marks and the needle now follow a real VU scale; v1.1 spaced −20 to +3 dB evenly. Clicking a meter switches both between gain reduction and input level; this replaces the meter swap button.
- **Lower CPU use with the window open**: the editor redraws only what changed. Meters keep their ballistics while audio plays. At rest the editor only checks the levels a few times a second and redraws nothing.
- Embedded fonts: Barlow Condensed, Inter, IBM Plex Mono, IBM Plex Sans Condensed, JetBrains Mono and Playfair Display, all under the SIL Open Font License 1.1. These replace Rajdhani.
- The version number changed, so hosts rescan the plugin and see its new parameters.

### Compatibility
- Projects and presets saved with v1.0 or v1.1 load unchanged. Projects saved with v1.2 still open in v1.1.

## 1.1 — 2026-03-27

- Needle VU meters with spring-mass-damper ballistics; GR needles with input bars, or input needles with GR bars.
- Optical cell retuned for audio-level response.
- Transformer: no more crossover artefacts; simpler waveshaping with a dry/wet mix, giving Nickel, Iron and Steel distinct but subtle characters.
- New layout: optical left, discrete right, meters in the centre, transformer and output at the bottom.

## 1.0 — 2026-03-27

First release: a dual-stage mastering compressor (a Port-Hamiltonian optical stage and a feed-forward VCA) followed by output transformer saturation (Nickel, Iron, Steel), with a sidechain HPF, stereo link and per-stage bypass. Available as VST3, AU and LV2.
