#import "../lib.typ": *

#set heading(numbering: "A.1")
#counter(heading).update(0)

= Limits

#table(
  columns: 2,
  [*Item*], [*Range*],
  [Beat rate], [8100 to 72000 bph],
  [Lift angle], [10° to 90°, default 52°],
  [cal], [−100.0 to +100.0 s/d],
  [Sample rates], [22.05, 32, 44.1 and 48 kHz, if the device supports them],
  [Trend recording], [one reading every 2 s, up to 100 hours],
  [Calibration], [about 15 minutes (900 ticks of a 1 Hz quartz reference)],
)

= Glossary

/ Amplitude: the swing of the balance wheel from its rest position, in degrees.
/ Beat: one tic or one toc. The balance makes two beats per oscillation.
/ Beat error: the positive timing asymmetry between the alternating Tic-to-Toc and Toc-to-Tic semiperiods, in ms.
/ bph: beats per hour.
/ cal: the correction for the sound card clock, in s/d.
/ dBFS: decibels relative to full scale. 0 dBFS is the loudest signal the
  input can record.
/ Lift angle: the angle over which the balance is in contact with the
  escapement, in degrees.
/ Paperstrip: the display of beat times, named after the paper charts of
  older timegraphers.
/ Positional error: the largest difference in average rate between
  positions.
/ Rate: the gain or loss of the watch, in s/d.
/ Snapshot: a frozen copy of a measurement, kept in its own tab.
/ .tgj: the file format of Tg, tg-timer and tg-expert.

= Credits and license

tg-expert is based on *Tg*, written by Marcello Mamino and the contributors of
#link("https://github.com/vacaboja/tg")[vacaboja/tg], and on *tg-timer*,
Alejandro Grigera's continuation of Tg. tg-expert is maintained by jefebus at
#link("https://github.com/jefebus/tg-expert")[github.com/jefebus/tg-expert].

tg-expert is free software, distributed under the GNU General Public
License, version 2. It comes with no warranty. The full license is in the
`LICENSE` file distributed with the program.

This manual is distributed under the same license.
