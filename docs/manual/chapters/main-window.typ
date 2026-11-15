#import "../lib.typ": *

= The main window <main-window>

#shot("main-window.png", [The main window while measuring a 28800 bph watch.])

The window has a top bar, a row of tabs, the results table, the paperstrip on
the left and, on the right, the waveform panels or the Trend page.

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
/ Take Snapshot: keeps the current measurement in a new tab. The shortcut is
  Ctrl+Space.
/ Current snapshot: the name of the selected snapshot tab, which can be
  edited.
/ Menu: #ui[Open], #ui[Save current display], #ui[Save all snapshots],
  #ui[Light algorithm], #ui[Audio settings…], #ui[Start calibration],
  #ui[User manual], #ui[Close all snapshots] and #ui[Quit].

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

The other lines, as listed in the legend at the bottom:

- *amplitude*: the pink line, read on the degree scale at the top;
- *rate guide*: the dashed blue lines lean with the measured rate, smoothed
  over about 10 seconds. When the dots run parallel to them, the rate is
  steady.

#shot("paper-strip.png", [The paperstrip with its controls. Tic and toc form
two close lines, and the amplitude falls from about 300° to 250°.],
width: 45%)

The buttons below the strip:

/ < and >: shift the strip by one grid division.
/ Clear: erases the strip and starts it over (live tab only).
/ Center: puts the newest beat in the middle of the strip.
/ −, 1x, +: zoom. The zoom levels are 0.25×, 0.5×, 1×, 2×, 4× and 8×.
  At 1× the strip is one beat wide. 1x returns to that level.

The settings below the buttons:

/ Y mark: the time between marks on the vertical axis, in seconds.
/ Y window: the time span shown vertically, in seconds. It is fixed and
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
the waveform.

A clean waveform with sharp, separate clicks means a good signal. A flat or
fuzzy waveform means the microphone isn't picking up the watch well.

The #ui[Trend] page is described in @trend.
