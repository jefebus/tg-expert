# TG-Expert - Feature Backlog
State values: Pending, Done, or Released in vx.x.x.

| Order | ID | Feature | Benefit | Cost | B/C | Minimum scope | State |
|---:|---|---|---:|---:|---:|---|---|
| 1 | TG-01 | Beat error graph | 3 | 1 | 3.00 | Add beat error (BE) as a third synchronized time-series graph using the values already recorded. | Done |
| 2 | TG-02 | CSV export | 5 | 2 | 2.50 | Export time, position, rate, amplitude, BE, validity, and measurement configuration. | Done |
| 4 | TG-04 | Settling period after a position change | 5 | 2 | 2.50 | Configurable countdown; retain settling samples but exclude them from accepted statistics. | Done |
| 5 | TG-05 | Explicit gaps and invalidity reasons | 5 | 2 | 2.50 | Show discontinuities and record why data is missing, without graphically joining invalid intervals. | Pending |
| 6 | TG-06 | Explicit paperstrip scale readings | 2 | 1 | 2.00 | Show the total phase interval, milliseconds per division, and visible duration; retain them in snapshots. | Pending |
| 7 | TG-07 | WAV/FLAC audio input | 5 | 3 | 1.67 | Open a recording as an alternative to live input: play/pause, restart, progress, channel selection, and explicit source calibration. | Pending |
| 8 | TG-08 | Regression tests with recordings | 5 | 3 | 1.67 | Process a fixed corpus and compare results with versioned expectations and defined tolerances. | Pending |
| 9 | TG-09 | Inspectable lift-time detection | 5 | 3 | 1.67 | Show start, end, threshold, duration, and separate tick/tock estimates; flag ambiguous starts. | Pending |
| 10 | TG-10 | Position segments and per-interval statistics | 5 | 3 | 1.67 | Preserve each visit to a position; select intervals and exclude samples reversibly. | Pending |
| 11 | TG-11 | Guided positional test | 5 | 3 | 1.67 | Configurable sequence with manual confirmation, settling, timed acquisition, and summary. | Pending |
| 12 | TG-12 | Rate vs. amplitude graph | 3 | 2 | 1.50 | Scatter plot colored by position or time and filterable by interval. | Done |
| 13 | TG-13 | Waveform comparison | 3 | 2 | 1.50 | Overlay tick/tock, freeze, share scales, and measure intervals with cursors. | Pending |
| 14 | TG-14 | Autosave and recovery | 4 | 3 | 1.33 | Incremental persistence, recovery after failure, and explicit clean shutdown. | Pending |
| 15 | TG-15 | Before/after comparison | 4 | 3 | 1.33 | Compare sessions with equivalent scales, configuration, and positional summaries. | Pending |
| 16 | TG-16 | Editable Typst service report and PDF export | 4 | 3 | 1.33 | Generate a portable editable report project using an installed, locally copied Typst template, JSON data, and SVG graphs. Independently select long-term data, current state, and selected snapshot diagnostics. Include project identification and notes, make owner inclusion explicit, automate non-blocking PDF compilation, preserve editable sources on failure, and never overwrite user edits silently. Re-estimate benefit/cost after implementation planning. | Pending |
| 17 | TG-27 | Provisional calibration estimate | 4 | 3 | 1.33 | Show an interim calibration estimate marked with `~`; if calibration is stopped before completion, ask whether to use that estimate. | Done |
| 18 | TG-17 | Input audio recording | 5 | 5 | 1.00 | Save audio before analysis in WAV/FLAC with metadata and calibration provenance. | Pending |
| 19 | TG-18 | File seeking, looping, and accelerated analysis | 5 | 5 | 1.00 | Seek with analysis reset and warm-up, repeat intervals, and process faster than real time. | Pending |
| 20 | TG-19 | Independent quality per measurement | 5 | 5 | 1.00 | Separate validity and diagnostic reasons for rate, BE, and amplitude; avoid unvalidated confidence percentages. | Pending |
| 21 | TG-20 | Configuration epochs without destructive clearing | 5 | 5 | 1.00 | Preserve the session when settings change and start a labeled configuration segment. | Pending |
| 22 | TG-26 | Algorithm configuration dialog | 3 | 3 | 1.00 | Expose algorithm parameters in a configuration dialog and provide defaults. | Done |
| 23 | TG-21 | Beat-aligned acoustic waterfall | 4 | 5 | 0.80 | One row per event, tick/tock separation, selectable alignment, and envelope intensity. | Pending |
| 24 | TG-22 | Extended power-reserve test | 3 | 4 | 0.75 | Configurable duration, winding/start marker, and distinction between a possible stop and signal loss. | Pending |
| 25 | TG-25 | Positional polar/radar view | 2 | 3 | 0.67 | Optional summary with numeric values and clearly defined scales. | Pending |
| 26 | TG-23 | Linked historical inspection | 5 | 8 | 0.62 | Select a point in trends and open the corresponding trace events, individual waveform, and audio. | Pending |
| 27 | TG-24 | Uncertainty validation and reference comparison | 5 | 8 | 0.62 | Quantify bias, repeatability, and failures with controlled signals and reference measurements. | Pending |
| 28 | TG-28 | Project metadata and snapshot notes in TGJ | — | — | — | Extend the existing tagged TGJ format with optional project title/reference, watch brand/model/reference, movement, serial number, owner, last-service date, and general notes; add independent snapshot notes. Preserve reading of older files and safely skip unknown fields. | Pending |
| 29 | TG-29 | Project and snapshot notes editor | — | — | — | Add a hideable, resizable notes panel that can remain visible alongside graphs. Include optional project/watch fields, plain-text project notes, and independent initially empty snapshot notes. | Pending |
| 30 | TG-30 | Complete-session save/open workflow | — | — | — | Replace partial-save actions with New session, Open, Save, and Save As. Persist project metadata, notes, the available current diagnostic state and history, and all snapshots, regardless of the selected tab. Clearly distinguish saved state from live acquisition. | Pending |
| 31 | TG-31 | Shared document rendering themes | — | — | — | Reuse the graph renderer for vector SVG output with screen, document-color, and monochrome themes. Preserve scales, data selection, gaps, and positional symbols; render legends consistently without widget screenshots. | Pending |
| 32 | TG-32 | Cross-platform Typst setup assistance | — | — | — | Detect and validate Typst, allow executable selection, and provide platform-specific installation assistance. Any managed download must be explicit and verified. Keep TG-Expert and editable-report export usable without Typst. | Pending |
| 33 | TG-33 | Live headphone monitoring | — | — | — | Optionally monitor unprocessed live input through an explicitly selected output, with independent volume and stable buffering. Help assess pickup placement, noise, and clipping. Low latency is not required; protect acquisition, prevent unbounded delay, disable on output failure, and never replay files or snapshots. | Pending |