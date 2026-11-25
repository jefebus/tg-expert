#import "../lib.typ": *

= Files <files>
Measurements are saved in files with the `.tgj` extension.

== Sessions
A session is one `.tgj` file with everything measured on a watch: the project details, the watch, its runs and its snapshots (see @runs). The title bar shows the project name and the file; `*` means there are unsaved changes.

- #ui[New session] starts an empty session for the watch selected in the top bar.
- #ui[Open session…] replaces the current session with the one in a file.
- #ui[Save] writes the whole session to its file, whichever tab is shown. The first time, it works as #ui[Save as…].
- #ui[Save as…] writes the session to another file, which becomes the session file.

Before a session is replaced, or the program closed, with unsaved changes, the program asks whether to save them. New runs and snapshots, renamed snapshots, changed positions, closed snapshots and the project details are changes; while recording in a session that has runs, snapshots or a file, new live readings are changes too. Watching the real-time tab while paused is not.

== Runs and snapshots <runs>
The program records in *runs*. A run is a period of continuous recording: it starts with #ui[Record] and ends with #ui[Pause], both on the button next to #ui[Take Snapshot]. The title of the real-time tab shows the state: #ui[Real time ● Run 2] while recording, #ui[Real time ❚❚ paused] while not.

- Starting the program and #ui[New session] start recording run 1. Opening a session leaves it paused.
- #ui[Pause] ends the run. It goes to a tab of its own, #ui[Run 1], #ui[Run 2]…, with its Trend and Correlations pages and the real-time tab as it was at the end. Its measurement is kept as it is: it cannot be closed or moved to another position. It can be given a name in the #ui[Current run] field, shown after its number: #ui[Run 2 after adjustment].
- #ui[Record] starts a new run. Its Trend starts from zero: runs are separate measurements and are never joined, even when they follow each other. After a break of more than 10 minutes a notice says how long it was.
- While paused, the real-time tab still analyses the sound, so that the watch and the microphone can be placed, but nothing is recorded and no snapshot can be taken.

A *snapshot* keeps the moment: readings, paper strip and waveforms. It does not stop the recording, so it has no Trend of its own; the Trend belongs to its run, which its tab tooltip names. Snapshots saved by older versions with a Trend keep it.

Snapshots are numbered within their run: #ui[2.1], #ui[2.2]… are the first and second snapshots of run 2, followed by the position when one is set, as in #ui[2.3 (DU)]. A name typed in the #ui[Current snapshot] field goes after the number: #ui[2.3 after adjustment (DU)]. The number is kept: closing a snapshot does not renumber the others. Snapshots of files from older versions, which have no run, keep their name.

The tabs are in order after #ui[Real time]: first the snapshots of the run being recorded, then the newest run, followed by its snapshots, then the run before it, and so on. In each group the newest snapshot comes first. Snapshot tabs show a camera. Snapshots of files from older versions, which have no run, come last.

Each run and snapshot has its own notes, written on the #ui[Measurement] page of the notes panel (see @notes-panel). On the real-time tab that page holds the notes of the run being recorded: they go with the run to its tab on #ui[Pause], and into the file on #ui[Save].

== Saving
#ui[Save] writes the runs, the snapshots and the run being recorded, up to that moment, with the real-time tab as it is then. Saving again writes the run in progress again: it is not duplicated. While calibrating, the real-time tab is not saved.

The save dialog proposes `YYYY-MM-DD project name.tgj` (the date of today), or the watch name when there is no project name. Characters that filenames cannot hold become `_`; the project name stored in the session is not changed. The `.tgj` extension is added when necessary.

The file is written to a temporary file in the same folder and only replaces the previous one once it has been written completely. If saving fails, the previous file stays as it was and the session keeps its unsaved changes.

#note[Use short, filesystem-safe names if the files will be moved between Windows, Linux and macOS.]

== Opening
- #ui[Open session…] in the #ui[File] menu;
- double-click a `.tgj` file on systems where the file type is registered (the Windows installer and `make install` do this);
- give a file on the command line: `tg-expert file.tgj`. A session is one file: when several are given, only the first one is opened.

Each run and each snapshot opens in its own tab, and the session is paused. The measurements of the file are kept as they are: they cannot be moved to another position or closed. Runs and snapshots can still be renamed; the name is not part of the measurement. A run cannot be resumed, since the internal buffers are not saved.

To go on measuring, for example after the program closed unexpectedly, press #ui[Record]: a new run starts, with the next number, and new snapshots belong to it. The next #ui[Save] keeps everything from the file and adds the new run and snapshots.

Opening a session does not change the watch selected in the top bar, its bph or lift angle, or the calibration. A notice says how the watch of the session relates to the saved watches: #ui[Use this watch] selects it to continue measuring with its settings. If the saved watch now has other settings than the session, or does not exist, the notice says so and nothing is changed.

== The watch of a session
The watch of a session is the one selected in the top bar. Choosing another watch when the session already has snapshots or a file asks whether to start a #ui[New session] for it or to #ui[Associate] this session with it. The snapshots keep their names and their own bph and lift angle either way.

== What a file contains
For each run and snapshot:
- the snapshot name and watch position;
- the date and time it was taken, or when the run started and ended (files from older versions do not have them);
- the results: rate, amplitude, beat error, bph, lift angle and cal;
- the beats shown on the paperstrip, including tic/toc identity and amplitude history;
- the averaged waveforms;
- paperstrip zoom and offset;
- for a run, its number and its long-term statistics and Trend graphs; for a snapshot, the run it was taken in and its number within that run;
- its notes.

The file does not contain the audio recording.

== Report (PDF)
#ui[Export report…] in the #ui[File] menu (Ctrl+P) writes a PDF report of the session, on A4 pages. The dialog lists the runs as the tabs do, the newest first, including the run being recorded (#ui[Run 3 (in progress)]). Under each run:

- #ui[Trends]: what the Trend and Correlations pages show for the run: the statistics, the rate, amplitude and beat error graphs and the three correlation graphs, over the whole run.
- #ui[Final state]: the real-time tab as it was when the run ended, or when the session was saved. For the run being recorded it is the #ui[State when exporting]. It is checked by default only for runs without snapshots.
- its snapshots, one check box each.

Unchecking a run leaves out everything under it. Below the runs:

- #ui[Current state]: the real-time tab as it is when you export: readings, paper strip and waveforms. It is unchecked by default, and not available without a valid measurement or while calibrating.
- #ui[Other snapshots (older files)]: the snapshots of files saved before runs existed, when there are any.
- #ui[Table of the runs on the first page].

The graphs are in colour or in black and grey (#ui[Monochrome]), for printing. The owner is only included when #ui[Include the owner] is checked.

The report is laid out like this:

- Every page has a header with the program logo, the report title (the project name, or the watch), the job reference and the date.
- The first page has a panel with the project name, job reference, watch, last service date and report date, when they are set, the general notes and the table of the runs: for each run its name, period, duration, positions measured, mean rate, mean amplitude, mean beat error and number of snapshots, and under it a line for each of its snapshots in the report, with its position, date and time and readings.
- Then each chosen run, in the order recorded (run 1 first), from a new page: its number and name, period and duration, its notes, its Trends, its final state and its snapshots. The final state and each snapshot are on a page of their own: first its notes, then the readings, paper strip and waveforms, and when it was taken.
- At the end, the current state and the other snapshots.

#shot("pdf-export.png", [Example of the first page of an exported PDF report.],
width: 85%)

The data is taken when you press #ui[Export], so the report shows one moment even while the measurement goes on. The report is written to a temporary file first and only replaces an existing file once it is complete. Exporting never changes the session or its notes.

The bph, lift angle and cal shown are those each measurement was taken with. The waveforms of a snapshot are the averaged sound of its own analysis window, not of a whole test. The report is not a certificate of conformity with any standard.

=== Notes in the report
Notes are written as plain text and shown in the report with this part of Markdown:

- a blank line starts a new paragraph; a single line break is kept;
- `#`, `##` and `###` at the start of a line make headings;
- `**bold**`, `*italic*` and `_italic_`;
- lines starting with `-`, `*` or `+` and a space, or with a number and a dot (`1.`), make lists.

Anything else (links, tables, code…) is shown as written: no text is left out.

== Compatibility
Files saved by Tg and tg-timer open in tg-expert. Snapshots saved without long-term statistics show a message on the Trend page instead of statistics. Opening a tg-expert file in an older program may discard Trend data when the file is saved again.

A file can also hold project details (project name, job reference, watch label, owner, date of the last service and general notes) and notes for each snapshot. These fields are optional. Files without them open as before, and older programs skip them when reading, but discard them if they save the file again. Older programs show a run as one more snapshot, named #ui[Run 1], #ui[Run 2]… in the language it was saved in. In files saved by tg-expert before runs existed, each #ui[Saved state] opens as a run. Snapshots of a run saved before snapshot numbers existed are numbered in the order they were taken when the file is opened. Files saved with older versions open as a session without project details; their snapshots are never taken as the name of a watch. The watch label is stored as text in the file: opening a file never adds, changes or applies a saved watch, and never changes the calibration.
