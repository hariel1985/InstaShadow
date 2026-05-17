# Contributing

Thanks for your interest in contributing! This is a small hobby project, but bug reports, feature ideas and pull requests are all welcome.

## Reporting a bug

Open an issue on GitHub with:

- **What you expected** vs **what happened**
- Your **OS** and **DAW** (with version)
- Plugin format you used (VST3 / AU / LV2) and the plugin **version** (visible in the editor)
- Steps to reproduce — the smaller and more specific, the better. A short audio example or a screenshot of the editor often saves a lot of back and forth.
- For crashes: any host log output, and whether the same project also crashes with the plugin removed.

Search existing issues first — if you find a match, add a comment with your details rather than opening a duplicate.

## Suggesting a feature

Open an issue describing **what** you'd use it for, not just **how** it should look. The "why" is what helps decide whether a feature fits the plugin's scope.

Plugins in the Insta* family aim to stay focused — each one solves one specific job well. If a feature would turn this plugin into a different plugin, it probably belongs in a new project.

## Submitting a pull request

The repository is public, so anyone can fork and open a PR. The `main` branch is protected: merges go through review.

1. **Fork** the repository on GitHub and clone your fork.
2. Create a feature branch from `main`:
   ```bash
   git checkout -b fix/short-description
   ```
3. Make your changes. Keep PRs small and focused — one feature or bug fix per PR makes review much faster.
4. **Build locally** before submitting:
   ```bash
   git clone --depth 1 https://github.com/juce-framework/JUCE.git ../JUCE
   cmake -B build -G "Visual Studio 17 2022" -A x64    # Windows
   cmake -B build -G Xcode                              # macOS
   cmake -B build -DCMAKE_BUILD_TYPE=Release             # Linux
   cmake --build build --config Release
   ```
   At minimum, the project must build cleanly with no new warnings on the platform you developed on. CI (GitHub Actions) will build on Windows, macOS and Linux automatically.
5. **Test it in a real DAW** — load the plugin in REAPER, Bitwig, Cubase, Logic, Ardour, or whatever you use, and verify the change behaves under typical conditions. Bypass tests, parameter-automation tests, and state-save/recall tests catch the majority of regressions.
6. **Commit** with a clear message (see below).
7. **Push** and open a PR against `main`. Fill in the PR template if there is one; otherwise describe what changed and why.

CI must pass before a PR can be merged. If your change is non-trivial, expect questions and possibly requests for changes before merging — that's normal.

## Code style

The project follows the surrounding code rather than any external style guide. A few concrete points:

- **C++17**, JUCE 8.
- **Brace style**: opening brace on its own line for functions; same line for control flow. Look at any existing file for a template.
- **Spaces, not tabs**. 4 spaces per indent.
- Member function naming: `lowerCamelCase` for methods, `lowerCamelCase` for variables, `UpperCamelCase` for types.
- Prefer `juce::` types in plugin code (`juce::String`, `juce::AudioBuffer<float>`, etc.) over `std::` equivalents where JUCE has one — the code is more consistent with the rest of the framework and the DSP helpers compose better.
- DSP code runs on the audio thread — **no allocations, no locks, no logging, no exceptions** in the audio path. Parameter updates use atomics; cross-thread state changes use lock-free patterns (the existing DSP classes under `Source/` show the conventions used in this codebase).
- GUI code uses JUCE `Component` + `LookAndFeel` + `Timer` patterns. Don't add per-frame allocations in `paint()`.
- Add a brief comment for non-obvious DSP — especially anything that depends on a specific filter topology or scaling convention.

## Commit messages

- One concise imperative-mood line at the top (under ~72 chars).
- Blank line, then a body that explains **why** if it isn't obvious from the diff. Wrap at ~72 chars.
- Reference issues with `Fixes #123` / `Closes #123` if applicable.

Example:

```
Fix Linear Phase auto-safety mono collapse on big FIRs

The smoothing attack on safety produced quantum-crossing changes too
frequently. Combined with the FIR builder's wait() being interrupted
by every notify(), the convolution could not finish preparing one IR
before the next arrived. Replace with a hard wall-clock debounce
that scales with FIR size.
```

## Licensing

This project is licensed under **GPL-3.0-or-later**. By submitting a pull request you agree to license your contribution under the same terms.

If you copy code from another project, **only GPL-compatible sources are acceptable** (GPL, LGPL, MIT, BSD, Apache 2.0 if the file headers permit). Note the original license and link to the source in a comment above the copied code. Any contribution that mixes in code under a stricter or incompatible license cannot be accepted.

## Questions

If you're not sure whether something is in scope before doing the work, open an issue first and ask. It saves everyone time.
