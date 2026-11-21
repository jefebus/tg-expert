#import "../lib.typ": *

= Long-term measurement <trend>

The rate of a watch changes as the mainspring unwinds and as the watch moves
between positions. The #ui[Trend] page records the measurement over hours, so
that these changes become visible.

#shot("trend.png", [The Trend page after measuring in three positions: dial
up, dial down and crown up.])

== Recording

Every 2 seconds, while the signal is good, the program records the rate,
amplitude and beat error, with the selected position. Periods without a
good signal are left out.

The recording starts over when a setting that changes the results changes:
bph, lift angle, cal, the audio input, the sample rate or the light
algorithm. #ui[Reset] starts it over by hand. Recording stops after 100 hours.

With #ui[BEAT] set to #ui[guess], the program can briefly recognize another
beat rate while the watch is turned or the signal is weak. Those readings are
left out, and the recording continues. Only a guessed beat rate that stays
different for a minute starts the recording over. For long recordings, set
the beat rate by hand.
Nothing is recorded during calibration.

== Settling after a position change

On the live Trend page, set #ui[Settling time (s)] (default 0, which disables
it). The setting is saved between sessions and changes apply to
the next countdown.

Select the new position in the software *before* physically moving the
watch. When changing from a named position to another position, including
#ui[not set], the countdown begins immediately and runs even without a good
signal. Returning to a previously used position starts it again. The first
selection from #ui[not set] does not start a countdown. A position already
selected when recording starts does not start one either. #ui[Reset] does
not start or restart a countdown; an active one continues.

Good-signal readings during the countdown are retained, shaded in the
graphs, and marked #ui[Settling] in CSV exports. They are excluded from
overall and per-position statistics, including positional error and graph
averages. The *now* column still shows the latest reading. Snapshots save
the settling markers; older application versions may ignore them.

== Statistics since the start

The table at the top shows, for rate (s/d), amplitude (°) and beat error
(ms):

- *now*: the latest reading;
- *avg*: the average;
- *min* and *max*;
- *spread*: max minus min;
- *σ*: the standard deviation.

Below it, the page shows how long the recording has run, the share of that
time with a good signal, and the bph, lift angle and cal used.

== Statistics per position

When the position was set during the recording, a second table shows, for
each position: the time spent in it, and the average rate, amplitude and beat
error. Readings taken with no position set are listed as #ui[not set].

With two or more positions, the page shows the *positional error*: the
largest difference between the average rates of the positions. A small
positional error means the watch is well adjusted.

== Graphs

At the bottom, three graphs share a time axis: rate, amplitude and beat error.
Each has a dashed line for its average when that average is within the visible
range. The beat error graph picks its vertical range automatically. A vertical
line with the position code (DU, DD, ...) marks each change of position. The
controls below the page set:

/ Rate: the vertical range of the rate graph: #ui[Auto], ±10 s/d or
  ±50 s/d.
/ Show: the time shown: #ui[All], or the last 1, 6 or 24 hours.

== Exporting data

Choose #ui[Export Trend as CSV] in the command menu to save the Trend data
from the active tab. Time values are elapsed seconds from the start of the
recording, not clock times. The file includes position codes and the
measurement settings. Only readings with a good signal are recorded, so
omitted periods appear as gaps; rows are marked valid or settling. Amplitude
is empty when it was not available.

The algorithm is marked #ui[Light (custom)] or #ui[Full (custom)] if any
measurement parameter used in that mode differs from its default. Calibration
parameters do not count, nor do noise suppression parameters in Light mode.
Snapshots retain this information; older snapshots without it are exported as
#ui[Light] or #ui[Full].

Choose portable CSV (comma separator and decimal point) or, when the
interface is in Spanish or German, localized CSV (semicolon separator and
decimal comma). The selected format is saved and reused for later exports,
including after restarting the program. Column names and descriptive values
follow the interface language; position codes remain the standard codes.
Both formats use UTF-8 with a byte order mark so Excel recognizes accented
characters.

== Measuring a power reserve

1. Wind the watch fully and set the position.
2. Start the measurement and leave the watch undisturbed.
3. Come back after some hours. The amplitude graph shows the amplitude
  falling as the mainspring unwinds, and the rate graph shows how the rate
  follows it.
4. Take a snapshot and save it. The Trend data is saved with it.
#note[*Before changing settings:* take a snapshot and save it if you need to keep the current Trend recording. Changing bph, lift angle, cal, audio input, sample rate, or algorithm settings resets the Trend data. For long recordings, enter the known bph instead of relying on automatic detection.]
