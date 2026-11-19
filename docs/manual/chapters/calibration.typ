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

Calibration takes about 15 minutes (900 ticks). Repeat it once to confirm
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
