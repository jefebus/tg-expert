#import "../lib.typ": *

= Introduction

tg-expert listens to a mechanical watch through the computer's audio input
and measures how the movement runs. It does the job of a timegrapher, using a
microphone instead of a dedicated sensor.

== What a timegrapher measures

A mechanical movement makes two sounds per oscillation of the balance wheel,
called the *tic* and the *toc*. Each beat is a group of short clicks from the
escapement. From the timing of these beats the program derives:

/ Rate: how much the watch gains or loses, in seconds per day (s/d). A rate
  of +5 s/d means the watch gains 5 seconds every day.
/ Amplitude: how far the balance wheel swings from its rest position, in
  degrees. A healthy, fully wound movement typically shows 270° to 310° dial
  up. Low amplitude often points to a weak mainspring, dirty oil or wear.
/ Beat error: the difference in time between the tic and the toc, in
  milliseconds. Zero means the balance is centered ("in beat").
/ Beat rate (bph): the number of beats per hour the movement is designed
  for, such as 18000, 21600, 28800 or 36000. The rate is computed against this
  value, so it must be right.
/ Lift angle: the angle, in degrees, over which the balance is in contact
  with the escapement. It is a property of the movement, published by the
  maker. Amplitude is computed from it, so a wrong lift angle gives a wrong
  amplitude. Rate and beat error do not depend on it.

== What tg-expert can and can't do

tg-expert can:

- measure rate, amplitude and beat error in real time, in any position;
- guess the beat rate of an unknown movement;
- show the beats on a paperstrip, so that irregularities are visible;
- record rate, amplitude and beat error for up to 100 hours, with
  statistics per watch position;
- keep snapshots of measurements and save them to files.

Compared with a hardware timegrapher:

- *The sensor matters.* A hardware timegrapher has a piezo sensor clamped to
  the watch. A microphone also picks up room noise, so the setup needs more
  care (see @microphone).
- *The clock must be calibrated.* A hardware timegrapher has its own
  reference oscillator. tg-expert relies on the sound card clock, which can be
  off by several seconds per day. Calibration (see @calibration) measures this
  error and corrects for it.
- *No positioning stand.* You set the watch in each position yourself and
  tell the program which position it is in.
- *Accuracy follows the setup.* With a good microphone placed against the
  case, a quiet room and a calibrated sound card, readings are repeatable to
  a fraction of a second per day. With a poor setup the numbers can be
  misleading.
