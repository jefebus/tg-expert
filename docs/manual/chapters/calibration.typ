#import "../lib.typ": *

= Calibration <calibration>

== Why

A computer contains several internal electronic clocks. 
The main board and the processor use their own clock references, 
but the most important one for tg-expert is the clock in the sound 
card or audio interface. It determines the actual audio sampling 
frequency and therefore the time scale used by the program to 
time the watch beats.

These clocks are usually derived from inexpensive quartz crystals. 
Their error may be several tens of parts per million (ppm), and it 
changes slightly with temperature. One ppm means one part per million; 
as a time error, it corresponds to 0.0864 seconds per day. For example, 
10 ppm is 0.864 s/d, while 50 ppm is 4.32 s/d. Small as it may seem,
this error is added directly to the RATE calculated by tg-expert.

Calibration measures the effective error of the sound card clock 
against a quartz reference and expresses it in s/d. The program then 
subtracts this value from subsequent measurements, so that RATE 
represents the watch under test and not the error of the computer's time base.

== How

You need a quartz watch or clock that ticks once per second, such as an
analog quartz watch with a seconds hand. Its accuracy becomes the accuracy of
the calibration, typically better than 0.5 s/d.

1. Place the quartz watch very close to the microphone, ideally touching it.
2. Work in a quiet room. Switch off fans and speakers nearby, and don't touch
  the desk.
3. Choose #ui[Start calibration] in the menu. The right side of the window
  shows the calibration instructions.
4. Watch the status in the results table:
  - *cal (wait)*: no steady tick once per second found yet. Check the setup.
  - *cal (acq.) NN %*: collecting ticks. Leave everything untouched.
  - *cal (done) ±X.X s/d*: finished. The value is copied to the #ui[cal]
    field and saved.
  - *cal (fail)*: the signal was too irregular. Improve the setup and start
    again.
5. When it shows *done*, choose #ui[Stop calibration] in the menu to return to
  normal measurement.

With default algorithm settings, calibration takes about 15 minutes (900 ticks).
Other algorithm settings can change the duration. Repeat it once to confirm
the result. Two runs should agree within a few tenths of a second per day.

#shot("calibration.png", [Calibration in progress, at 5 %. The paperstrip
shows the reference ticks, and the right side shows the instructions.])

== How long it lasts

The cal value belongs to the sound card and its clock, not to the watch. It
is saved and used at every start. Calibrate again:

- when you change the audio input or the sample rate;
- when you use another computer or sound card;
- if the room temperature is very different, since the crystal drifts a
  little with temperature;
- from time to time, to check it. Once a year is a reasonable habit.

The cal value can also be typed in the #ui[cal] field, if it is known from an
earlier calibration.

== Expert calibration controls <calibration-parameters>

Normally you do not need to change these values. If you understand their
effect and need to do so, stop calibration first and open
#ui[Algorithm configuration…]. On the #ui[Calibration] tab,
#ui[Edit calibration settings (Expert)] enables the six controls.
Unchecking it locks them without discarding or applying edits.
Apply and Save work as described in @algorithm-configuration: analysis
restarts and the INI stores only non-default values. Keep editing enabled
if you want Reset to defaults to include these controls.

These settings belong to the quartz reference, not the movement you will
measure afterwards. The reference still ticks once per second; the program
combines those sounds into a one-second waveform and follows their timing
drift to calculate the correction. Check placement and quiet surroundings
using the procedure above before adjusting detection.

#table(
  columns: (2.8fr, 1.2fr, 0.65fr, 0.95fr),
  [Parameter], [INI key], [Default], [Range],
  [Observation count], [`cal_count`], [900], [300–3600],
  [Calibration edge threshold (%)], [`cal_edge`], [5], [3–10],
  [Minimum observation interval (s)], [`cal_interval`], [0.9], [0.5–2],
  [Fit acceptance precision (s/day)], [`cal_precision`], [0.1], [0.1–0.5],
  [Lower position in folded second (%)], [`cal_lock_low`], [40], [35–45],
  [Upper position in folded second (%)], [`cal_lock_high`], [60], [55–65],
)

/ Observation count:
  This is how many valid observations are collected before calculating
  the fit. It is not a duration in seconds: increasing the count usually
  means keeping the reference undisturbed for longer.
/ Calibration edge threshold:
  Helps locate the start of the reference pulse. The threshold is placed
  above the surrounding background maximum by adding this percentage
  of the gap to the folded pulse peak. It is neither absolute microphone
  volume nor a position within the second.
/ Minimum observation interval:
  Prevents observations from being accepted too close together. More than
  this time must pass since the previous one. It does not guarantee that
  update rate: missed pulses or a longer interval extend collection.
/ Fit acceptance precision:
  This is the uncertainty allowed for accepting calibration. The calculated
  uncertainty must fall below it, in s/day. Raising it permits a less
  precise fit, but neither improves the reference nor sets the correction
  that will be saved.
/ Lower position in folded second:
  Marks the beginning of the region that may contain the reference peak;
  the selected rising edge must come after this position. The usual 40%
  means 400 ms into the one-second waveform, not the time between observations.
/ Upper position in folded second:
  Closes the allowed region for the reference peak. The default 60% means
  600 ms into the one-second waveform. Together with the lower bound,
  it defines where the peak is expected, rather than adding another
  edge-detection threshold.
