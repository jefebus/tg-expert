# Changelog

## Unreleased

### Changed

- A traditional menu bar (File, Settings, Tools, Help) replaces the menu button at the right of the top bar. Calibration is in Tools; Help has the user manual and an About dialog with the version, licence and a link to the GitHub repository.
- Project and notes panel floating over the right part of the tabs (hidden by default, shown from Tools, closed with its own button, resizable from its left edge): project name, job reference, owner, last-service date, the watch of the session, general notes, and independent notes for each snapshot. Changes go straight into the session.
- Correlation and Trend graphs: readings beyond a fixed rate or beat error range are no longer drawn on the border as if read there; the line breaks and they are counted ("Off scale: N") above the graph. The paper strip amplitude line also breaks where the amplitude is beyond its scale.
- Waveform panels: each column shows the peak of the samples it covers, so that narrow pulses are not lost.
- The graphs (correlations, Trend, waveforms, paper strip) are drawn by renderers shared with the coming PDF report, with screen, document colour and document monochrome themes.
- PDF report (File > Export report…, Ctrl+P), written directly with Cairo on A4: header with the project and watch, trends (statistics, Trend and correlation graphs), current state and chosen snapshots with paper strip, waveforms and notes. Colour or monochrome; the owner only on request. Notes are rendered with a Markdown subset (headings, bold, italic, lists). Written through a temporary file; the session is not changed. Requires Cairo with its PDF backend and PangoCairo (both come with GTK 3).
- Keyboard shortcuts: Ctrl+N new session, Ctrl+O open, Ctrl+S save, Ctrl+Shift+S save as, Ctrl+E export data, Ctrl+Q quit, F1 user manual (Ctrl+Space still takes a snapshot).
- Sessions: the File menu has New session, Open session…, Save, Save as… and Export data…, replacing Open, Save current display, Save all snapshots, Export Trend as CSV… and Close all snapshots. Save writes the whole session (project, watch, all snapshots and the real-time tab as "Saved state") whichever tab is shown, through a temporary file that replaces the previous one only when complete. Opening a session replaces the current one and lets it go on; measurements loaded from the file are kept as they are. Unsaved changes are shown with `*` in the title and asked about before they are lost.
- Choosing another saved watch when the session already has measurements asks whether to start a new session or associate the session with that watch. Opening a session never selects, applies, adds or changes saved watches or the calibration.

- Recording runs: a Record / Pause button next to Take Snapshot. The program and New session start recording; an opened session is paused. Pause closes the run into its own tab with its Trend and correlations; Record starts a new run whose Trend starts from zero. While paused the live display still analyses the audio, but nothing is recorded and no snapshot can be taken. Snapshots no longer copy the Trend: they keep the moment (readings, paper strip, waveforms) and name the run they were taken in. Tabs are ordered newest run first, each followed by its snapshots (newest first, with a camera icon); they are no longer reordered by dragging.
- Tab titles by number: runs are "Run N"; snapshots are "N.M" (the M-th snapshot of run N), with an optional name after the number and the position, e.g. "2.3 after adjustment (DU)". New snapshots are no longer named after the selected watch. Closing a snapshot does not renumber the others.
- Run notes: the notes panel page "Snapshot" is now "Measurement" and edits the notes of the selected run or snapshot; on the real-time tab, those of the run being recorded, which go with it to its tab on Pause and into the file on Save.
- PDF report organised by run: the first page adds a table of the runs (period, duration, positions, mean rate, amplitude and beat error, number of snapshots); then each run in the order recorded, from a new page, with its notes, Trends, final state and snapshots (one per page); then the current state and the snapshots of older files. The export dialog is a tree: each run with its Trends, final state (checked by default only for runs without snapshots) and snapshots, then current state (unchecked by default), other snapshots and the summary table. The "From" selector is gone: each run has its own Trends.
- Runs can be named like snapshots ("Run 2 after adjustment"). The summary table on the first page of the report lists, under each run, its snapshots in the report with their position, date and time and readings. In the report the notes of a run, its final state and each snapshot come first, before the readings and graphs.

### Compatibility

- A run's name is stored in an optional `run_name` field; its `name` holds the whole title ("Run 2 after adjustment") for older versions and is not used when reading.
- Snapshots store their number within the run in an optional `snap` field. Snapshots of runs in files without it are numbered in file order when opened; older versions skip the field.

- Runs are marked with an optional `role` field ("run"), with optional `run` and `run_start` fields; older versions show them as normal snapshots. The "Saved state" entries of earlier 0.11 development files open as runs. Snapshots saved with a Trend by older versions keep it.
- Snapshots store when they were taken in an optional `wall_time` field (seconds since 1970, UTC), shown in the PDF report in local time. Files from older versions have no date; older versions skip the field.

- `.tgj` files can store optional project metadata (project name, job reference, watch label, owner, last-service date as `YYYY`, `YYYY-MM` or `YYYY-MM-DD`, general notes) in a `project-metadata` block inside `data`, and optional `notes` for each snapshot. Text is UTF-8, up to 256 bytes per field and 64 KiB per note. Older files open unchanged; older versions skip the new fields when reading and drop them when saving.

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
