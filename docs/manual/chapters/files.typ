#import "../lib.typ": *

= Files <files>
Measurements are saved in files with the `.tgj` extension.

== Sessions
A session is one `.tgj` file with everything measured on a watch: the project details, the watch, all the snapshots and the state of the real-time tab when it was saved. The title bar shows the project name and the file; `*` means there are unsaved changes.

- #ui[New session] starts an empty session for the watch selected in the top bar.
- #ui[Open session…] replaces the current session with the one in a file.
- #ui[Save] writes the whole session to its file, whichever tab is shown. The first time, it works as #ui[Save as…].
- #ui[Save as…] writes the session to another file, which becomes the session file.

Before a session is replaced, or the program closed, with unsaved changes, the program asks whether to save them. New snapshots, renamed snapshots, changed positions, closed snapshots and the project details are changes; once the session has snapshots or a file, new live readings are changes too. Just watching the real-time tab is not.

== Saving
#ui[Save] writes the snapshots and, as #ui[Saved state], what the real-time tab shows at that moment, with its long-term statistics. Saving again during the same run replaces that saved state with the current one. While calibrating, the real-time tab is not saved.

The save dialog proposes `YYYY-MM-DD project name.tgj` (the date of today), or the watch name when there is no project name. Characters that filenames cannot hold become `_`; the project name stored in the session is not changed. The `.tgj` extension is added when necessary.

The file is written to a temporary file in the same folder and only replaces the previous one once it has been written completely. If saving fails, the previous file stays as it was and the session keeps its unsaved changes.

#note[Use short, filesystem-safe names if the files will be moved between Windows, Linux and macOS.]

== Opening
- #ui[Open session…] in the #ui[File] menu;
- double-click a `.tgj` file on systems where the file type is registered (the Windows installer and `make install` do this);
- give a file on the command line: `tg-expert file.tgj`. A session is one file: when several are given, only the first one is opened.

Each snapshot opens in its own tab, and the saved state in a tab named #ui[Saved state]. The measurements of the file are kept as they are: they cannot be moved to another position or closed. Snapshots can still be renamed; the name is not part of the measurement. They cannot be resumed as live measurements either, since the internal buffers are not saved.

The session can go on after opening, for example after the program closed unexpectedly. The real-time tab measures as usual, its long-term statistics start from zero, and new snapshots are added to the session. The next #ui[Save] keeps everything from the file, including its #ui[Saved state], and adds the new snapshots and a new saved state. A session can thus hold several saved states, one for each part of the recording; each keeps its own statistics and settings, and they are not joined.

Opening a session does not change the watch selected in the top bar, its bph or lift angle, or the calibration. A notice says how the watch of the session relates to the saved watches: #ui[Use this watch] selects it to continue measuring with its settings. If the saved watch now has other settings than the session, or does not exist, the notice says so and nothing is changed.

== The watch of a session
The watch of a session is the one selected in the top bar. Choosing another watch when the session already has snapshots or a file asks whether to start a #ui[New session] for it or to #ui[Associate] this session with it. The snapshots keep their names and their own bph and lift angle either way.

== What a file contains
For each snapshot:
- the snapshot name and watch position;
- the results: rate, amplitude, beat error, bph, lift angle and cal;
- the beats shown on the paperstrip, including tic/toc identity and amplitude history;
- the averaged waveforms;
- paperstrip zoom and offset;
- long-term statistics and the graphs from the Trend page.

The file does not contain the audio recording.

== Compatibility
Files saved by Tg and tg-timer open in tg-expert. Snapshots saved without long-term statistics show a message on the Trend page instead of statistics. Opening a tg-expert file in an older program may discard Trend data when the file is saved again.

A file can also hold project details (project name, job reference, watch label, owner, date of the last service and general notes) and notes for each snapshot. These fields are optional. Files without them open as before, and older programs skip them when reading, but discard them if they save the file again. Older programs show a saved state as one more snapshot, named #ui[Saved state] in the language it was saved in. Files saved with older versions open as a session without project details; their snapshots are never taken as the name of a watch. The watch label is stored as text in the file: opening a file never adds, changes or applies a saved watch, and never changes the calibration.
