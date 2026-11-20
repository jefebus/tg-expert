#import "../lib.typ": *

= Troubleshooting <troubleshooting>

== No input

*The message "No audio input could be opened." appears at start.*

- Open the audio settings and choose another audio system or input device.
- Check that the operating system sees the microphone, and that it isn't
  muted or disabled.
- Check that no other program uses the input exclusively.

*The meter in the audio settings doesn't move.*

- The wrong device is selected, or its gain is at zero. Tap the microphone
  and see whether the meter reacts.

*Windows: microphone access.* Windows 10 and 11 can block desktop programs
from the microphone. Open *Settings > Privacy & security > Microphone* and
turn on both *Microphone access* and *Let desktop apps access your
microphone*. Then restart tg-expert.

*Windows: WDM-KS inputs.* The WDM-KS audio system lists many inputs, and
some of them can't be opened, or open but deliver silence. If an input under
WDM-KS fails, choose the same device under WASAPI or MME instead. MME is the
most compatible choice.

*Windows: "enhancements".* Some drivers apply noise suppression or automatic
gain to the microphone. Turn them off in the device properties in the Windows
sound settings.

*Linux: device busy.* If JACK or PipeWire-JACK is running, ALSA devices may
be busy. Choose JACK as the audio system, or stop JACK.

*macOS: no input.* Allow microphone access in *System Settings > Privacy &
Security > Microphone* for tg-expert or for the terminal that started it.

== Wrong bph

*The BEAT column shows a value marked with \~, or the rate is absurd (hundreds
of s/d).*

- With #ui[guess], the program may lock on the wrong beat rate when the
  signal is weak. Set the bph by hand if it is known.
- If the bph is set by hand, check it against the movement's data. 21600 and
  28800 are the most common values. An 18000 bph movement set to 21600 shows
  a rate of thousands of s/d.
- The status icon shows *!* when there is no valid rhythm at the selected
  bph.

== Unstable readings

*The rate jumps around, or the paperstrip is scattered.*

- Improve the signal first: move the microphone into contact with the case,
  reduce room noise, and adjust the gain. See @microphone.
- Check the waveform panels. Sharp, separate clicks mean a good signal.
- Turn off the #ui[Light algorithm] in #ui[Algorithm configuration…] if it is on.
- Don't touch the watch or the desk during the measurement.
- Some instability is the watch itself. A worn or dirty movement varies from
  beat to beat. The Trend page's σ shows how much.

*The amplitude is missing or implausible.*

- Check the lift angle.
- Amplitude needs a reliable acoustic lift-time measurement: the unlocking onset and the strong final locking/banking reference must be detectable. A weak or noisy signal may give a stable rate but no reliable amplitude.

*The rate is consistently off compared with the watch's real behavior.*

- Calibrate the sound card (see @calibration), and calibrate again after
  changing the audio input or the sample rate.
