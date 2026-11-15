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

At the bottom, two graphs share a time axis: the rate, with a dashed line at
its average, and the amplitude. A vertical line with the position code (DU,
DD, ...) marks each change of position. The controls below the page set:

/ Rate: the vertical range of the rate graph: #ui[Auto], ±10 s/d or
  ±50 s/d.
/ Show: the time shown: #ui[All], or the last 1, 6 or 24 hours.

== Measuring a power reserve

1. Wind the watch fully and set the position.
2. Start the measurement and leave the watch undisturbed.
3. Come back after some hours. The amplitude graph shows the amplitude
  falling as the mainspring unwinds, and the rate graph shows how the rate
  follows it.
4. Take a snapshot and save it. The Trend data is saved with it.
