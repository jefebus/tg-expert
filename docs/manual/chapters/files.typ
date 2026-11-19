#import "../lib.typ": *

= Files <files>
Measurements are saved in files with the `.tgj` extension.

== Saving
- #ui[Save current display] saves the selected tab: either the live measurement or one snapshot.
- #ui[Save all snapshots] saves all snapshot tabs in a single file.

The save dialog proposes a filename from the measurement context. When a saved watch has been selected, its name is used as the base. If a position is set, the position code is added. When several snapshots are saved together, use the session/watch name rather than the name of only one position. You can edit the proposed filename before saving. The `.tgj` extension is added when necessary.

#note[Use short, filesystem-safe watch and snapshot names if the files will be moved between Windows, Linux and macOS. Renaming a snapshot changes the name stored inside the `.tgj` file; the filename can still be chosen independently in the save dialog.]

== Opening
- #ui[Open] in the menu;
- double-click a `.tgj` file on systems where the file type is registered (the Windows installer and `make install` do this);
- give one or more files on the command line: `tg-expert file.tgj`.

Each snapshot in the file opens in its own tab. Snapshots can be viewed, renamed, assigned a position and saved again, but they cannot be resumed as live measurements.

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
