#import "../lib.typ": *

= Measuring a watch <measuring>

== Setting bph and lift angle

1. Look up the movement's beat rate and lift angle. Movement data sheets and
  watchmaking references list both. Many common movements use a lift angle
  between 50° and 53°. The default of 52° is a reasonable guess when the
  value is unknown, but the amplitude is then approximate.
2. Set #ui[BEAT] to the beat rate, or choose #ui[guess]. Guessing works for
  most watches, but a wrong guess is possible with a weak signal. When the
  beat rate is known, set it.
3. Set #ui[L.A.] to the lift angle.

Changing bph, lift angle or cal restarts the long-term statistics.

== Saved watches

For a watch you measure often, set its bph and lift angle and press the save
button next to the watch list. Give it a name. Selecting the watch later sets
both values. The delete button removes the selected watch from the list.

== Positions

A watch runs differently in different positions, because gravity acts on the
balance differently. Timegraphers use these standard positions:

#table(
  columns: 2,
  [*Code*], [*Position*],
  [DU], [Dial up],
  [DD], [Dial down],
  [CU], [Crown up],
  [CD], [Crown down],
  [CL], [Crown left],
  [CR], [Crown right],
)

Select the position in the top bar before placing the watch in it. The
position is recorded with the readings and the snapshots. Wait about a
minute after turning the watch, so the readings can settle.

== Snapshots

#ui[Take Snapshot] (Ctrl+Space) keeps the current measurement in a new tab.
The tab is named after the selected watch and shows the position. A snapshot
keeps the results table, the paperstrip, the waveforms and the Trend page as
they were.

In a snapshot tab:

- the name can be changed in the #ui[Current snapshot] field;
- the position can be changed in the top bar;
- the tab is closed with the × in its title.

A typical session: measure dial up, take a snapshot, turn the watch crown
down, wait, take another snapshot, and so on. Then save the session: one
file holds all the snapshots (see @files).
