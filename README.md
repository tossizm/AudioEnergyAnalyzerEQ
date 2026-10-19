# AudioEnergyAnalyzerEQ VST3 (Windows)

A JUCE/CMake source project for a 10-band parametric EQ plug-in. GitHub Actions builds a Windows x64 VST3 bundle and publishes it as a downloadable workflow artifact.

## Contents

- `Source/PluginProcessor.*` — audio processing, ten peaking filters, host automation/state.
- `Source/PluginEditor.*` — ten gain controls and a JSON reference loader.
- `config/Reference_Average_Default.json` — example ten-band reference.
- `.github/workflows/windows-vst3.yml` — automated Windows build and artifact upload.
- `scripts/build_windows_vst3.bat` — local Windows build script.

## GitHub: build and download

1. Open this repository's **Actions** tab and select **Build AudioEnergyAnalyzerEQ VST3 (Windows)**.
2. Push a commit, or choose **Run workflow**. Wait for the green successful run.
3. Open the completed run and download the artifact named `AudioEnergyAnalyzerEQ-Windows-VST3`. The artifact ZIP contains the `.vst3` bundle and example JSON.

The workflow uses a Windows runner with Visual Studio 2022. Do not change it to `ubuntu-latest`, because the build uses the Visual Studio generator.

## Build locally on Windows 11

Install Visual Studio 2022 with **Desktop development with C++**, CMake 3.22+, and Git. Double-click `scripts/build_windows_vst3.bat`. On first build CMake fetches JUCE 8.0.12 from its official GitHub repository; internet access is required.

## Loading the VST3

The workflow artifact ZIP contains `Tossizm\\AudioEnergyAnalyzerEQ.vst3` plus the reference JSON. Close your DAW, then copy the entire `Tossizm\\AudioEnergyAnalyzerEQ.vst3` bundle folder into `C:\\Program Files\\Common Files\\VST3\\Tossizm\\`. The expected binary path is `C:\\Program Files\\Common Files\\VST3\\Tossizm\\AudioEnergyAnalyzerEQ.vst3\\Contents\\x86_64-win\\AudioEnergyAnalyzerEQ.vst3`. Do not copy only the inner `.vst3` binary. Restart the DAW and run a plug-in rescan.

## JSON integration: important limitations

The JSON loader supports a root object containing a non-empty `bands` array. Each band can specify a zero-based `index` (0–9; 1–10 is also accepted), and one of `target_db`, `gain_db`, `energy_percent`, `target_percent`, or `ratio`. A band can alternatively be identified by its exact `min_hz`/`max_hz` pair. Values are converted to approximate gain and clamped to ±12 dB. **A percentage distribution is not itself an EQ curve**: this conversion is an approximate starting point, not a perceptually or acoustically exact inverse-EQ calculation. Use the ten controls to fine-tune. Match the loader to the exact JSON schema exported by your installed AudioEnergyAnalyzer version before relying on automatic corrective EQ.

## Build status and scope

This repository contains source and automation, not a precompiled binary. The VST3 is created by GitHub Actions when you push/run the workflow. A successful C++ build and DAW load test must be confirmed from the actual Actions run and your host; source changes alone do not prove that.

## Licensing

JUCE is fetched from its upstream repository at build time. Review JUCE's current licensing terms for your intended distribution. Add your own license before public redistribution if needed.
