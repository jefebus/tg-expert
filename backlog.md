# TG-Expert - Feature Backlog
| Order | ID | Feature | Benefit | Cost | B/C | Minimum scope |
|---:|---|---|---:|---:|---:|---|
| 1 | TG-01 | Beat error graph | 3 | 1 | 3.00 | Add beat error (BE) as a third synchronized time-series graph using the values already recorded. |
| 2 | TG-02 | CSV export | 5 | 2 | 2.50 | Export time, position, rate, amplitude, BE, validity, and measurement configuration. |
| 3 | TG-03 | Calibration profiles per input | 5 | 2 | 2.50 | Associate a correction with the device, backend, and sample rate; save the date and warn if no profile matches. |
| 4 | TG-04 | Settling period after a position change | 5 | 2 | 2.50 | Configurable countdown; retain settling samples but exclude them from accepted statistics. |
| 5 | TG-05 | Explicit gaps and invalidity reasons | 5 | 2 | 2.50 | Show discontinuities and record why data is missing, without graphically joining invalid intervals. |
| 6 | TG-06 | Explicit paperstrip scale readings | 2 | 1 | 2.00 | Show the total phase interval, milliseconds per division, and visible duration; retain them in snapshots. |
| 7 | TG-07 | WAV/FLAC audio input | 5 | 3 | 1.67 | Open a recording as an alternative to live input: play/pause, restart, progress, channel selection, and explicit source calibration. |
| 8 | TG-08 | Regression tests with recordings | 5 | 3 | 1.67 | Process a fixed corpus and compare results with versioned expectations and defined tolerances. |
| 9 | TG-09 | Inspectable lift-time detection | 5 | 3 | 1.67 | Show start, end, threshold, duration, and separate tick/tock estimates; flag ambiguous starts. |
| 10 | TG-10 | Position segments and per-interval statistics | 5 | 3 | 1.67 | Preserve each visit to a position; select intervals and exclude samples reversibly. |
| 11 | TG-11 | Guided positional test | 5 | 3 | 1.67 | Configurable sequence with manual confirmation, settling, timed acquisition, and summary. |
| 12 | TG-12 | Rate vs. amplitude graph | 3 | 2 | 1.50 | Scatter plot colored by position or time and filterable by interval. |
| 13 | TG-13 | Waveform comparison | 3 | 2 | 1.50 | Overlay tick/tock, freeze, share scales, and measure intervals with cursors. |
| 14 | TG-14 | Autosave and recovery | 4 | 3 | 1.33 | Incremental persistence, recovery after failure, and explicit clean shutdown. |
| 15 | TG-15 | Before/after comparison | 4 | 3 | 1.33 | Compare sessions with equivalent scales, configuration, and positional summaries. |
| 16 | TG-16 | Printable service report | 4 | 3 | 1.33 | Watch identification, configuration, calibration, results by position, graphs, and notes. |
| 17 | TG-27 | Provisional calibration estimate | 4 | 3 | 1.33 | Show an interim calibration estimate marked with `~`; if calibration is stopped before completion, ask whether to use that estimate. |
| 18 | TG-17 | Input audio recording | 5 | 5 | 1.00 | Save audio before analysis in WAV/FLAC with metadata and calibration provenance. |
| 19 | TG-18 | File seeking, looping, and accelerated analysis | 5 | 5 | 1.00 | Seek with analysis reset and warm-up, repeat intervals, and process faster than real time. |
| 20 | TG-19 | Independent quality per measurement | 5 | 5 | 1.00 | Separate validity and diagnostic reasons for rate, BE, and amplitude; avoid unvalidated confidence percentages. |
| 21 | TG-20 | Configuration epochs without destructive clearing | 5 | 5 | 1.00 | Preserve the session when settings change and start a labeled configuration segment. |
| 22 | TG-26 | Algorithm configuration dialog | 3 | 3 | 1.00 | Expose algorithm parameters in a configuration dialog and provide defaults. |
| 23 | TG-21 | Beat-aligned acoustic waterfall | 4 | 5 | 0.80 | One row per event, tick/tock separation, selectable alignment, and envelope intensity. |
| 24 | TG-22 | Extended power-reserve test | 3 | 4 | 0.75 | Configurable duration, winding/start marker, and distinction between a possible stop and signal loss. |
| 25 | TG-25 | Positional polar/radar view | 2 | 3 | 0.67 | Optional summary with numeric values and clearly defined scales. |
| 26 | TG-23 | Linked historical inspection | 5 | 8 | 0.62 | Select a point in trends and open the corresponding trace events, individual waveform, and audio. |
| 27 | TG-24 | Uncertainty validation and reference comparison | 5 | 8 | 0.62 | Quantify bias, repeatability, and failures with controlled signals and reference measurements. |
