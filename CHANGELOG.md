# Changelog

## 0.12.0

### Added

- Record separate measurement runs in one session—for example, before and
  after an adjustment. Each run has its own Trend graphs. You can name runs
  and snapshots, and snapshots stay grouped with the run they came from.
- Keep a job reference, last service date and general notes with the session.
  Add separate notes to individual runs and snapshots.
- Export an A4 PDF report from File > Export report… (Ctrl+P). Choose which
  runs and snapshots to include, print in colour or monochrome, and include
  the owner's name only if you choose to.

### Changed

- Save now keeps the whole session in one `.tgj` file, including watch
  details, runs, snapshots and notes. Reopened sessions start paused, and
  tg-expert warns you before discarding unsaved changes.
- A menu bar replaces the old menu button. Calibration is under Tools, and
  the user manual is under Help (F1).
- Graphs now show gaps where readings go off scale instead of plotting them
  at the edge. Narrow pulses are easier to see in the waveform display.

### Compatibility

- Existing `.tgj` files still open. Older versions may show runs as ordinary
  snapshots and may lose new session details if you save the file again
  from an older version.

## 0.11.0

Changes since 0.10.0:

### Added

- Live input listening through a selected audio output.
- A beat-error graph alongside the rate and amplitude Trend graphs.
- Trend export to portable or localized CSV, including recorded positions, validity and measurement settings.
- A configurable settling period after a position change. 
- Correlation scatter plots for rate, amplitude and beat error
- An algorithm configuration dialog with defaults, advanced and expert controls.
- A provisional calibration estimate that can be explicitly accepted when stopping calibration early.

### Changed

- Added more detailed feedback about measurement detection status.

### Compatibility

- Existing `.tgj` files remain supported. New Trend data may include settling markers and whether the algorithm was customized; older versions ignore these fields.

## 0.10.0

Spanish and German localization, expanded documentation, and improvements to
the paperstrip and long-term statistics.

### Added

- Spanish and German translations of the interface, including menus, dialogs,
  tooltips, panel titles, paperstrip legends and the Trend page. The language
  follows the system settings.
- User manuals in Spanish and German. "User manual" opens the manual in the
  preferred language when available, falling back to English.
- Illustrated manual chapter on the Swiss lever escapement: its mechanical
  phases, acoustic pulses, and how tg-expert derives rate, beat error and
  amplitude. Includes guidance on measurement reliability and the limits of
  interpreting the waveform.
- Paperstrip wrapping: dots crossing either vertical margin reappear at the
  opposite margin. The wrap interval follows the current millisecond scale
  and zoom, with duplicate dots near the margins where needed. Center
  continues to put the newest beat in the middle.

### Changed

- The paperstrip Y mark increases and decreases by 1 s with + and -, and by
  10 s with the page keys. Finer values can still be entered manually.
- The Y mark and Y window labels now show their unit, seconds.
- Manuals share a redesigned layout with consistent colors, tables,
  headings, note boxes and page footers.
- Expanded documentation on sound-card calibration, installation, language
  selection, saving measurements and Trend behavior in automatic bph mode.
- Manual version numbers are taken automatically from the project version.
- The Windows installer includes all three manuals, each with its own Start
  menu shortcut, plus Spanish and German interface translations.
- With Typst available, `make manual` builds all three manuals and
  `make install` installs them.
- Added gettext translation build and maintenance support. Builds without
  `msgfmt` remain available in English; Windows installer builds require
  the translation catalogs.

### Fixed

- With bph set to "guess", short-lived changes in the detected beat rate no
  longer erase the Trend statistics, for example while turning the watch
  or during a weak signal. Those readings are excluded. A different guessed
  beat rate must persist for one minute before restarting the recording.

### Compatibility

- No changes to the snapshot or configuration file formats.
- Existing `.tgj` files from Tg, tg-timer and tg-expert remain supported.
- Numbers continue to use a point as the decimal separator in every
  language. Results-table headers, position codes and calibration status
  indicators remain in English.

## 0.9.0

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
