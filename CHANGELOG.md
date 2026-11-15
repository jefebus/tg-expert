# Changelog

## 0.9.0 (unreleased)

First release under the name tg-expert.

### Added

- Audio settings dialog, with selection of the audio system (host API),
  input device and sample rate, and an input level meter.
- Saved watch presets (bph and lift angle), stored in the configuration file.
- Watch position of snapshots (DU, DD, CU, CD, CL, CR), selectable before or
  after taking the snapshot, shown in the tab title and saved in `.tgj` files.
  Older versions ignore the new field.
- Trend page with per-position statistics and positional error. Trend data is
  saved in `.tgj` files.
- Calibration: "Start/Stop calibration" menu item with a tooltip.
  Instructions replace the waveform panels while calibrating, and the
  settings and menu are locked until it ends.
- User manual (PDF), opened with "User manual" in the menu. The Windows
  installer includes it, with a Start menu shortcut. `make install` installs
  it in the documentation folder when Typst is available. Without the PDF,
  the menu item opens the releases page.
- Paperstrip Y mark and Y window settings.
- Ctrl+Space takes a snapshot.
- Tooltip for the light algorithm.
- Titles on all panels, and a ms time axis on the full-period panel.
- Audio status feedback, and a free bph estimate (marked with ~) when the
  selected bph doesn't match the signal.

### Changed

- Top bar grouped into watch (preset, bph, lift angle), audio (input, rate,
  cal) and snapshot controls.
- Results bar reworked to resemble hardware timegraphers.
- Color scheme and axis labels reworked for clarity.
- Paperstrip reworked: inverted direction, stable rate guide lines, legends,
  fixed width, new zoom levels, a shared ms and amplitude grid, and larger
  dots.
- Audio errors are reported as single, clear messages.
- The program, package, configuration file (`tg-expert.ini`), window title,
  man page (`tg-expert.1`) and Windows installer use the name tg-expert. The
  installer has its own identity and installs next to tg-timer.
- The configuration file starts fresh. Settings from `tg-timer.ini` are not
  imported.
- Removed the Travis CI configuration.

### Fixed

- Crash when capturing at 96 kHz.
- Slope of the rate lines on the paperstrip.
- The Trend statistics no longer reset when the watch is turned to another
  position.

### Compatibility

- `.tgj` files from Tg and tg-timer open unchanged.

## 0.8.0

Maintenance and backport release (tg-timer) focused on usability,
reliability and packaging.

- Added paperstrip zoom controls with improved centering and shift behavior.
- Added tic/toc-aware event rendering and amplitude history overlay.
- Added top-bar audio input device selector and sample-rate selector.
- Added automatic runtime fallback to 44.1 kHz when the selected rate is
  unsupported.
- Improved low-bph handling using a bph-aware period guard.
- Extended snapshot persistence for trace zoom, tic/toc metadata and
  amplitude history. Existing snapshots remain supported.
- The configuration persists `audio_device` and `audio_rate`.
- Hardened the Windows MSI packaging script with prerequisite checks and
  automatic WXS generation.
