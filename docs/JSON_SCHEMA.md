# Reference JSON contract currently implemented by the plug-in

The plug-in expects an object with a `bands` array. For each band, use one of:

- `index`: band number 0–9 (the loader also accepts 1–10 as one-based indices)
- or `min_hz` and `max_hz`: exact standard boundaries: 20–60, 60–120, 120–250, 250–500, 500–1000, 1000–2000, 2000–5000, 5000–10000, 10000–16000, 16000–20000 Hz
- and one of `target_db`, `gain_db`, `energy_percent`, `target_percent`, `ratio`

`target_db` and `gain_db` are interpreted as decibels. Percentage/ratio values are converted with `10 * log10(value / 10)` then clamped to ±12 dB. That conversion is only a rough mapping: energy percentages do not uniquely determine the EQ gain needed for a particular audio sample. The currently implemented parser is intentionally permissive and should be aligned to the actual ClubAudioAnalyzer exporter schema before production use.
