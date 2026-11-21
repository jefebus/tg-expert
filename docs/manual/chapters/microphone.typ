#import "../lib.typ": *

= The microphone <microphone>

Most bad readings come from the audio setup, not from the program: a weak or
noisy microphone, a microphone too far from the watch, or room noise.

== Which microphone

Better choices:

- a contact microphone or piezo pickup;
- an earbud or headset microphone pressed against the case;
- a decent USB microphone placed against the watch.

Usually worse:

- the built-in microphone of a laptop;
- a desk microphone at a distance;
- microphones with noise cancellation or automatic gain enabled.

A simple earbud microphone held against the case often works better than an
expensive microphone a few centimeters away.

== Placement

- Put the microphone in contact with the case, the caseback or the movement
  holder.
- Keep the watch and microphone still. A small holder or a clamp helps.
- If the signal is weak, try other spots on the case. Small changes in
  position can make a large difference.
- When measuring positions (see @measuring), check the signal again after
  turning the watch.

#shot("microphone-setup.jpg", [The in-line microphone of an earphone cable,
laid directly on the watch crystal.], width: 60%)

== Gain

Raise the gain until the beats are clearly visible in the waveform panels
and the input level meter moves well above the noise, without reaching
0 dBFS. Too much gain amplifies room noise and can clip the beats.

Turn off audio "enhancements" in the operating system: noise suppression,
automatic gain control and echo cancellation change the signal and disturb
the measurement.

== Noise

- Work in a quiet room. Switch off fans, speakers and noisy devices nearby.
- Don't touch the desk or the cables while measuring.
- A soft pad under the holder isolates the watch from desk vibrations.
- The #ui[Light algorithm] (under #ui[Algorithm configuration…], #ui[Basic]) skips the noise suppressor. Leave
  it off unless the computer is too slow.
