#import "../lib.typ": *

= Files <files>

Measurements are saved in files with the `.tgj` extension.

== Saving

- #ui[Save current display] saves the selected tab: the live measurement or
  one snapshot.
- #ui[Save all snapshots] saves every snapshot tab to one file.

== Opening

- #ui[Open] in the menu;
- double-click a `.tgj` file, on systems where the file type is registered
  (the Windows installer and `make install` do this);
- give one or more files on the command line: `tg-expert file.tgj`.

Each snapshot in the file opens in its own tab. Snapshots can be viewed,
renamed, given a position and saved again, but not continued.

== What a file contains

For each snapshot:

- the name and the watch position;
- the results: rate, amplitude, beat error, bph, lift angle and cal;
- the beats shown on the paperstrip, with tic and toc, and the amplitude
  history;
- the averaged waveforms;
- the paperstrip zoom and position;
- the long-term statistics and graphs of the Trend page.

The file doesn't contain the audio recording.

== Compatibility

Files saved by Tg and tg-timer open in tg-expert. Snapshots saved without
long-term statistics show a message on the Trend page instead of the
statistics. Files saved by tg-expert may lose the Trend data when opened in
the older programs.
