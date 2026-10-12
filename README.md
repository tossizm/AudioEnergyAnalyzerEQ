# ClubAudioAnalyzer EQ VST3 (Windows)

A JUCE/CMake source project for a 10-band parametric EQ plug-in. GitHub Actions builds a Windows x64 VST3 bundle and publishes it as a downloadable workflow artifact.

## Contents

- `Source/PluginProcessor.*` — audio processing, ten peaking filters, host automation/state.
- `Source/PluginEditor.*` — ten gain controls and a JSON reference loader.
- `config/Reference_Average_Default.json` — example ten-band reference.
- `.github/workflows/windows-vst3.yml` — automated Windows build and artifact upload.
- `scripts/build_windows_vst3.bat` — local Windows build script.

## GitHub: build and download

1. Create a GitHub repository named `ClubAudioAnalyzer-VST3`.
2. Upload/push the **contents** of this project folder to the repository root. Keep `.github/workflows/windows-vst3.yml` at exactly that path.
3. Open the repository's **Actions** tab and select **Build ClubAudioAnalyzer EQ VST3**.
4. Push a commit, or choose **Run workflow**. Wait for the green successful run.
5. Open the completed run and download the artifact named `ClubAudioAnalyzerEQ-Windows-VST3` at the bottom of the run summary. The artifact is a ZIP containing the `.vst3` bundle and example JSON.

The workflow uses `windows-2022`, which provides Visual Studio 2022 on the hosted runner. The verification step checks that the C++ toolchain is available before CMake configures the project. Do not use `ubuntu-latest` for this Visual Studio generator.

## Build locally on Windows 11

Install Visual Studio 2022 with **Desktop development with C++**, CMake 3.22+, and Git. Double-click `scripts/build_windows_vst3.bat`. On first build CMake fetches JUCE 8.0.12 from its official GitHub repository; internet access is required.

## Loading the VST3

Copy the built `ClubAudioAnalyzer EQ.vst3` bundle to `%CommonProgramFiles%\VST3` (typically `C:\Program Files\Common Files\VST3`) and rescan plug-ins in your DAW. Close the DAW before replacing an installed bundle.

## JSON integration: important limitations

The JSON loader currently supports a root object containing a non-empty `bands` array. Each band can specify a zero-based `index` (0–9; 1–10 is also accepted), and one of `target_db`, `gain_db`, `energy_percent`, `target_percent`, or `ratio`. A band can alternatively be identified by its exact `min_hz`/`max_hz` pair. Values are converted to approximate gain and clamped to ±12 dB. **A percentage distribution is not itself an EQ curve**: this conversion is a convenient initial mapping, not a perceptually or acoustically exact inverse-EQ calculation. Use the ten controls to fine-tune. The current loader is a starting integration and must be matched to the exact JSON schema exported by your installed ClubAudioAnalyzer version before relying on it for automatic corrective EQ.

## Build status and scope

This repository contains source and automation, not a precompiled binary. The VST3 is created by GitHub Actions when you push/run the workflow. A successful C++ build and DAW load test must be confirmed from the actual Actions run and your host; source generation alone does not prove that.

## Licensing

JUCE is fetched from its upstream repository at build time. Review JUCE's current licensing terms for your intended distribution. This project source is provided as a starting point; add your own license before public redistribution if needed.
