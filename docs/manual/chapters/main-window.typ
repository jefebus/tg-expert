#import "../lib.typ": *

= The main window <main-window>

#shot("main-window.png", [The main window while measuring a 28800 bph watch.])

The window has a top bar, a row of tabs, the results table, the paperstrip on
the left and, on the right, the waveform, Trend or Correlations page.

== The top bar

From left to right:

/ Watch: a list of saved watches. Selecting one sets its bph and lift angle.
  The save button stores the current bph and lift angle under a name, and the
  delete button removes the selected watch.
/ BEAT: the beat rate in bph. Choose a preset, type a value, or choose
  #ui[guess] to let the program find it.
/ L.A.: the lift angle in degrees, from 10 to 90.
/ Audio button: shows the audio input in use. Click it to open the audio
  settings.
/ cal: the calibration value in s/d, from −100 to +100 s/d. It is filled in by
  calibration, and it can also be typed in.
/ Position: the watch position (dial up, dial down, crown up, crown down,
  crown left, crown right, or not set).
/ Record / Pause: starts and ends a run, a period of continuous recording
  (see @runs).
/ Take Snapshot: keeps the current measurement in a new tab, without
  stopping the recording. Only while recording. The shortcut is Ctrl+Space.
/ Current snapshot: the name of the selected snapshot tab, which can be
  edited.
The menu bar at the top of the window has four menus:

/ File: #ui[New session], #ui[Open session…], #ui[Save], #ui[Save as…],
  #ui[Export data…], #ui[Export report…] (see @files) and #ui[Quit].
/ Settings: #ui[Audio settings…] and #ui[Algorithm configuration…].
/ Tools: #ui[Start calibration] (#ui[Stop calibration] while calibrating) and
  #ui[Project and notes panel], which shows or hides that panel.
/ Help: #ui[User manual] and #ui[About Tg Expert], with the version, the
  licence and a link to the source code on GitHub.

Keyboard shortcuts, shown next to the menu entries: Ctrl+N new session,
Ctrl+O open session, Ctrl+S save, Ctrl+Shift+S save as, Ctrl+E export data,
Ctrl+P export report,
Ctrl+Q quit, F1 user manual, and Ctrl+Space to take a snapshot. A shortcut
only works while its entry is available (for example, not while calibrating).

== Project and notes panel <notes-panel>
It floats over the right part of the tabs and covers them only partly, so
that most of the graphs stay in view while you write. It is hidden at first:
#ui[Project and notes panel] in the #ui[Tools] menu shows or hides it, and its
close button hides it. Drag the strip along its left edge to change its width.
The program remembers whether it was shown and its width. It has two pages:

/ Project: #ui[Project name], #ui[Job reference], #ui[Owner] and
  #ui[Last service] (a year, a month or a day: `YYYY`, `YYYY-MM` or
  `YYYY-MM-DD`, kept as typed; an invalid date is shown in red and not saved
  until it is valid), in the foldable #ui[Identification] group; the watch
  of the session, which is the one chosen in the saved watches list and is
  not edited here; and the #ui[General notes].
/ Measurement: the notes of the run or snapshot in the selected tab, with
  its title (#ui[Notes of: Run 2], #ui[Notes of: 2.3 (DU)]). Each run and
  each snapshot has its own notes, empty at first. On the real-time tab,
  while recording, they are the notes of the run being recorded
  (#ui[Notes of: Run 3 (in progress)]); they go with the run to its tab on
  #ui[Pause]. While paused no run is being recorded, and the page says so.

#shot("notes_project.png", [The Project page, with identification details and
general notes.], width: 60%)

#shot("notes_measurement.png", [The Measurement page, with notes for a
snapshot.], width: 65%)

What you type changes the session at once (there is no Apply button) and is
saved with it. Notes are plain text, up to 64 KiB each.

The detection process and the #ui[Algorithm configuration…] dialog are
described in @algorithm, with the complete parameter reference in
@algorithm-configuration.

== The results table

The table at the top of each tab shows:

/ RATE: the rate in s/d, corrected with the cal value.
/ AMP.: the amplitude in degrees, when it can be measured.
/ B.E.: the beat error in ms.
/ L.A.: the lift angle used.
/ BEAT: the beat rate used, in bph. A value marked with \~ is a free estimate,
  shown when the signal doesn't match the selected bph.

At the left of the table, a clock icon and four squares show the state of the
signal. Each square is one analysis window with a valid period. The clock
shows:

- *0*: no audio input;
- *?*: there is audio, but no beat rate has been recognized yet;
- *!*: no valid rhythm at the selected beat rate;
- the plain clock face: audio and a valid beat period are detected.

Hover over the icon to see this explanation in the program.

== The paperstrip

The paperstrip draws each beat as a dot. Time runs downwards. The horizontal
position of a dot is the time of the beat within the window, so a watch that
keeps time draws vertical lines. A watch that gains draws lines leaning one
way, and a watch that loses draws lines leaning the other way. Tic and toc are
drawn in different colors. When beat error is present, they form two separate
lines.

The paperstrip has a fixed width. Two scales run along the top and share the
same vertical grid lines: the amplitude in degrees, and the time within the
window in ms. The time labels on the left are minutes and seconds.

Dots crossing either vertical margin reappear at the opposite margin. The
wrap interval follows the current ms scale and zoom. Near a margin, a dot
can appear on both sides; Center still puts the newest beat in the middle.

The other lines, as listed in the legend at the bottom:

- *amplitude*: the pink line, read on the degree scale at the top;
- *rate guide*: the dashed blue lines lean with the measured rate, smoothed
  over about 10 seconds. When the dots run parallel to them, the rate is
  steady.

#shot("paper-strip.png", [The paperstrip with its controls. Tic and toc form
two close lines, with the amplitude history shown alongside.],
width: 45%)

The buttons below the strip:

/ < and >: shift the strip by one grid division.
/ Clear: erases the strip and starts it over (live tab only).
/ Center: puts the newest beat in the middle of the strip.
/ −, 1x, +: zoom. The zoom levels are 0.25×, 0.5×, 1×, 2×, 4× and 8×.
  At 1× the strip is one beat wide. 1x returns to that level.

The settings below the buttons:

/ Y mark (s): the time between marks on the vertical axis, in seconds. + and
  − change it by 1 s. Smaller values, such as 0.5, can be typed.
/ Y window (s): the time span shown vertically, in seconds. It is fixed and
  does not change by itself.

== The waveform panels

The #ui[Waveforms] page on the right shows the averaged sound of the beats:

/ Tic: the averaged sound of the first beat.
/ Toc: the averaged sound of the second beat.
/ Full period: the tic and toc together, with the two highlighted.
/ Debug: the processed signal used by the analysis.

#shot("waveform-panels.png", [The waveform panels. Each beat shows the
separate clicks of the escapement.])

The tic and toc panels have a ms scale at the bottom and an amplitude scale in
degrees at the top. The blue vertical line marks the amplitude measured from
the waveform. The view expands to the left when needed to include the
configured minimum amplitude; the highlighted
windows in the full-period panel expand to match. This changes only the
display, not the amplitude detection.

A clean waveform with sharp, separate clicks means a good signal. A flat or
fuzzy waveform means the microphone isn't picking up the watch well.
The status line below the waveforms stays in place when empty. Hover over a
truncated message to see its full text.

When the beat is detected but amplitude cannot be measured, the status below
the waveforms says "Amplitude unavailable". Rate and beat error remain
available; the beat error is not refined using lift-pulse timing. The debug
build also shows the reason for the amplitude rejection.

If a stable period is found but tic and toc cannot be separated, the display
switches automatically to "Rate only": beat error and amplitude are shown as
unavailable, and no new waveform, paperstrip events, or Trend readings are
produced until tic and toc can be detected again. Earlier trace and Trend
history remain visible.

The #ui[Trend] and #ui[Correlations] pages are described in @trend.
