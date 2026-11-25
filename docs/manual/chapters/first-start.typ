#import "../lib.typ": *

= First start

On start, tg-expert opens the default input device of the default audio
system. If no input can be opened, it shows the message #ui[No audio input
could be opened.] with a button to the audio settings.

== The audio settings dialog

Open it with #ui[Audio settings…] in the #ui[Settings] menu, or by clicking the audio button in the top bar, which shows the
input currently in use.

#shot("audio-settings.png", [The audio settings dialog. The line near the
bottom shows the input in use.], width: 55%)

/ Audio system: the sound system used for recording and live listening. On
  Windows these are MME, DirectSound, WASAPI and WDM-KS. On Linux, ALSA, and
  JACK if it is running. Choosing another audio system selects its default
  input and the default rate. The output device list follows this system too.
/ Input device: the microphone or line input to record from.
/ Output device: the device used to listen live. Only devices from the selected
  audio system are listed.
/ Sample rate: the rate at which the input is recorded. Only the rates the
  device accepts are listed, out of 22.05, 32, 44.1 and 48 kHz. 44.1 kHz is
  a good default. A higher rate isn't always better, especially with a noisy
  microphone: use the lowest rate that gives a stable reading.
/ Input level: a meter showing the level of the incoming signal in dBFS,
  updated while the dialog is open.

Changes take effect at once: the audio input restarts, and the measurement
starts over. The choice is saved and used at the next start.

Audio settings can't be changed during calibration, or while a snapshot tab
is selected. The dialog says so when that is the case.

== Checking the input level

Hold the watch against the microphone and watch the meter.

- If the meter barely moves, the wrong device is selected or the gain is too
  low. Raise the microphone gain in the operating system's sound settings.
- If the meter stays near 0 dBFS, the input is overloaded. Lower the gain.
- Tapping the microphone should make the meter jump. If it doesn't, the
  program is listening to a different device.

The status indicator next to the clock in the results table tells you
whether the program recognizes a beat. See @main-window.

#note[Calibrate the sound card before trusting the rate (see @calibration). The cal value is retained between sessions. Recalibrate after changing the audio input or sample rate; see @calibration for the other cases.]
