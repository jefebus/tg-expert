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

#shot("microphone-setup.jpg", [A piezo microphone housed in a case for
watch measurement with USB interface.], width: 60%)

== Gain

Raise the gain until the beats are clearly visible in the waveform panels
and the input level meter moves well above the noise, without reaching
0 dBFS. Too much gain amplifies room noise and can clip the beats.

Turn off audio "enhancements" in the operating system: noise suppression,
automatic gain control and echo cancellation change the signal and disturb
the measurement.

== Live listening

Choose #ui[Listen LIVE] to hear the current microphone input through the
selected output. Use headphones: TG-Expert cannot detect whether headphones
are physically connected, and speakers can feed back into the microphone.
Listening follows the live input even while a snapshot tab is selected; it
never plays snapshot or historical audio.

Choose the output and adjust its independent listening volume in #ui[Audio
settings]. The volume is deliberately limited and does not change input gain
or improve the signal used for measurement. If the selected output disappears,
listening stops and must be reactivated after choosing an available output.
Some systems can switch one physical jack between headphones and speakers
without changing its device identity, which the program cannot detect.

Listen for the clock ticks relative to room noise, compare different pickup
positions and contact pressure, and notice knocks or cable rubbing. Possible
clipping should also be checked against the input level meter. Listening is
only a setup aid; it does not automatically diagnose the movement.

== Noise

- Work in a quiet room. Switch off fans, speakers and noisy devices nearby.
- Don't touch the desk or the cables while measuring.
- A soft pad under the holder isolates the watch from desk vibrations.
- The #ui[Light algorithm] (under #ui[Algorithm configuration…], #ui[Basic]) skips the noise suppressor. Leave
  it off unless the computer is too slow.
