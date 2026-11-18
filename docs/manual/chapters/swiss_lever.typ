#import "../lib.typ": *

= Interpreting the Sound of the Swiss Lever Escapement

This chapter explains what the waveform displayed by tg-expert represents, how it relates to the operation of the Swiss lever escapement, and what the algorithm actually uses to calculate rate, beat error, and amplitude. It also provides practical criteria for determining whether a measurement is reliable.

The main idea is that a tic or a toc is not a single impact. Each beat contains several mechanical actions occurring very close together. Three main acoustic pulses can usually be identified in the sound. tg-expert processes the complete acoustic pulse train and extracts repeatable timing references from it.

== Escapement Operation and Formation of the tic-toc Sound

The Swiss lever escapement connects the going train, which delivers energy from the mainspring, to the regulating organ formed by the balance wheel and hairspring. During each beat, the balance impulse jewel moves the pallet fork, the escape wheel is unlocked, an impulse is transmitted, and the escape wheel is locked again. During the following beat, the same sequence is repeated on the opposite pallet stone.

=== The Five Mechanical Phases

#table(
  columns: (25mm, 50mm, 50mm, 50mm),
  inset: 5pt,
  stroke: rgb("B7C1CA"),
  align: (left, left, left, left),
  table.header(
    [*Phase*], [*What happens*], [*Likely acoustic signature*], [*Diagram*]
  ),

  [*1. Unlocking*],
  [The impulse jewel contacts the pallet fork. The pallet fork starts to rotate and the escape-wheel tooth leaves the locking face of the pallet stone.],
  [First sound, normally well defined in time but weaker than the final sound.],
  [#shot("fase_1.png", [], width: 50%)],

  [*2. Impulse*],
  [The tooth travels along the impulse face. Energy is transferred from the escape wheel to the pallet stone, pallet fork, impulse jewel, and finally to the balance wheel.],
  [A distributed and irregular pulse that may merge with the unlocking sound.],
  [#shot("fase_2.png", [], width: 50%)],

  [*3. Drop*],
  [At the end of the impulse, the escape wheel rotates freely through a small angle until the next tooth reaches the opposite pallet stone.],
  [The free motion itself produces little sound. The sound commonly associated with the drop is produced mainly at the end of this phase.],
  [#shot("fase_3.png", [], width: 50%)],

  [*4. Locking*],
  [The next tooth strikes the locking face of the opposite pallet stone and stops the escape wheel.],
  [A strong and abrupt impact.],
  [#shot("fase_4.png", [], width: 50%)],

  [*5. Run to the Banking Pin*],
  [After the initial locking contact, the tooth settles into lock and the pallet fork continues through a small angle until it reaches the banking pin.],
  [This occurs very close to locking. Locking and banking usually form one strong acoustic pulse train.],
  [#shot("fase_5.png", [], width: 50%)],
)

The exact sequence is:

#align(center)[
  #box(fill: rgb("F3F5F7"), inset: 9pt, radius: 3pt)[
    *unlocking -> impulse -> drop -> locking contact -> run to the banking pin*
  ]
]

Locking begins before the pallet fork reaches the banking pin. These are successive events, but the interval between them is very short and their vibrations overlap in a conventional acoustic recording.

=== The Three Acoustic Pulses

Watch timing literature usually describes three main acoustic pulses within each tic or toc:

+ *Unlocking.* The impulse jewel enters the pallet fork and starts the motion of the pallet fork. This first sound provides a precise timing reference, although it can be relatively weak.
+ *Impulse.* The escape-wheel tooth travels along the impulse face and the pallet fork transfers energy to the balance. The sound is less regular and does not always appear as an isolated peak.
+ *Locking, and banking.* The drop ends when the next escape-wheel tooth reaches the locking face. Immediately afterwards, the pallet fork completes its motion against the banking pin. This third pulse train is usually the strongest.

#shot("waveform_3_pulses.png", [Conceptual representation of the envelope of a tic or a toc.], width: 50%)

The actual waveform contains bipolar oscillations, rebounds, and resonance tails. A short mechanical impact excites many frequencies in the movement, bridges, main plate, case, crystal, and the sensor itself. Therefore, the height of each peak depends not only on the mechanical contact but also on the contact point, watch position, microphone pressure, and frequency response of the recording system.

#note(
  [Do not interpret each peak as a specific component: The waveform is the combined response of the escapement, movement, case, support, and microphone. The identification of unlocking, impulse, and locking is based on their timing. An individual oscillation in the waveform cannot be assigned unambiguously to a specific mechanical contact.]
)

=== Differences Between Tic and Toc

Tic and toc correspond to alternating actions of the entry and exit pallets. Their timing should ideally be symmetrical around the balance wheel's equilibrium position, but their acoustic shape and electrical amplitude may differ. Possible causes include geometrical differences between the pallet stones, lock depth, watch position, and unequal transmission of vibrations to the microphone.

A difference in peak height between tic and toc does not by itself indicate a fault. Timing stability, repeatability in each direction, and the correct separation between the two events are more significant.

=== Parameters to be extracted from the sound

#shot("parameters_from_sound.png", [How the parameters are extracted from the sound waveform.], width: 100%)

== How tg-expert Processes the Sound

The tg-expert algorithm is derived from the open-source tg program. It does not operate as a classifier of mechanical components or escapement phases. Instead, it converts the audio into an envelope, searches for periodic repetition of the pattern, and measures timing references.

=== Processing Chain

#table(
  columns: (12mm, 45mm, 99mm),
  inset: 5pt,
  stroke: rgb("B7C1CA"),
  table.header([*No.*], [*Operation*], [*Purpose*]),
  [1], [High-pass filter], [Reduces drift, slow vibrations, and handling noise.],
  [2], [Noise suppression], [Rejects sustained intervals with abnormally high energy.],
  [3], [Rectification], [`fabs()` converts the bipolar signal into a positive magnitude.],
  [4], [Low-pass filter], [Extracts the time-domain envelope of the acoustic pulse train.],
  [5], [FFT-based autocorrelation], [Finds periodicity without requiring a fixed tic waveform.],
  [6], [Cycle folding], [Aligns multiple ticks and tocks to create a representative waveform.],
  [7], [Trimmed mean], [Discards the highest 20% of values to reduce the effect of accidental impacts.],
  [8], [tic-toc separation], [Searches for the opposite action close to half the period.],
  [9], [Lift time], [Locates the detectable beginning of the acoustic pulse before each reference event.],
  [10], [Template correlation], [Locates the individual events used to generate the trace.],
)

=== Signal Conditioning and Envelope Extraction

The high-pass filter preserves fast impacts while reducing slow movement and drift. Rectification removes the sign of the vibration. The low-pass filter smooths the rectified signal and produces an energy envelope. Subsequent detection operates mainly on this envelope rather than on the individual high-frequency oscillations of the original audio signal.

This transformation improves robustness, but it also means that tg-expert does not attempt to explicitly separate unlocking, impulse, drop, locking, and banking.

=== Rate and Period

The program calculates the autocorrelation of the envelope. A periodic signal resembles itself when shifted by a number of samples equal to its period, so the autocorrelation contains repeated peaks.

When the nominal beat rate is specified in beats per hour, the algorithm centers the search around the expected period. A watch running at 28,800 A/h performs eight beats per second:

$ 28800 / 3600 = 8 " beats/s" $

The separation between one tic and the following toc is approximately:

$ 1 / 8 = 0.125 " s" $

The separation between two equivalent events, for example from one tic to the next tic, is approximately 0.25 s. Internally, the code uses this period between equivalent events and searches for the opposite event near the halfway point.

Rate measurement depends primarily on the pattern repeating consistently. A recording can therefore produce a convincing rate measurement even when the initial unlocking sound is not perfectly distinguishable.

#note(
  [A good rate measurement does not automatically validate the amplitude measurement. The algorithm can track periodicity using a strong and stable portion of the acoustic pulse train. Amplitude measurement, however, depends on correctly locating the timing boundaries of the lift interval.]
)

=== Beat Error

The algorithm searches for the tic-to-toc separation close to half the period between equivalent events. The difference between the measured separation and the ideal halfway point provides an initial estimate of beat error.

Physically, beat error indicates that the two alternating escapement actions are not centered in time around the equilibrium position of the balance. The program refines the result using the pulse durations detected in both directions.

=== Amplitude Calculation

Amplitude is not calculated from the height of the sound waveform. tg-expert estimates the time during which the balance passes through the lift angle and combines this time with the lift angle configured for the movement.

For approximately sinusoidal balance motion, the code applies:

$ A = L A / (2 sin(pi t_L / T)) $

Where:

- $A$ is the angular amplitude of the balance.
- $L A$ is the lift angle entered by the user.
- $t_L$ is the lift time estimated from the acoustic pulse train.
- $T$ is the period between equivalent events used by the algorithm.

For the same lift angle, a balance oscillating with a high amplitude crosses the central region *at a higher speed*. Therefore:

- a short lift time corresponds to a higher amplitude;
- a long lift time corresponds to a lower amplitude.

The algorithm smooths the waveform using a window of approximately 1 ms. It then calculates a threshold based on the global maximum and the background level. For each tic and toc, it searches backwards from the central reference until it finds the detectable beginning of the pulse train. It then follows the rising envelope to a local maximum and converts the detected position into a duration.

The program accepts the calculation if the tic and toc amplitudes are between 135° and 360° and differ by less than 60°. If the check fails, it increases the threshold and attempts to locate another pulse edge.

#note(
  [Effect of missing the unlocking pulse. If the first sound remains below the detection threshold, tg-expert starts measuring too late. The estimated lift time becomes shorter and the formula produces an artificially high amplitude. The resulting value may still appear reasonable if it passes the algorithm's internal plausibility checks.]
)

The lift angle is also critical. Because it is a direct multiplier in the equation, a 4% error in the entered lift angle produces approximately a 4% error in the calculated amplitude, assuming the measured lift time remains unchanged.

=== What the Algorithm Distinguishes

#table(
  columns: (70mm, 86mm),
  inset: 5pt,
  stroke: rgb("B7C1CA"),
  table.header([*Element*], [*Treatment in tg-expert*]),
  [tic versus toc], [Yes. The two directions are located and kept separate.],
  [Three acoustic pulses], [They are not classified individually. They form part of a composite envelope.],
  [Peak height], [It affects detection and thresholding, but does not determine amplitude.],
  [Lift time], [Yes. It is estimated using the detectable beginning of the acoustic pulse train.],
  [Balance amplitude], [It is inferred from lift time, period, and the configured lift angle.],
)

The internal variable name `tic_pulse` should not be interpreted as the physical “impulse phase”. It represents a time interval detected within the processed waveform.

== Evaluating Measurement Quality

Reliability should be assessed before interpreting the numerical results. The following table summarizes the main checks.

#table(
  columns: (35mm, 61mm, 60mm),
  inset: 5pt,
  stroke: rgb("B7C1CA"),
  table.header([*Indicator*], [*Good measurement*], [*Questionable measurement*]),
  [Background], [Low and stable between beats.], [Continuous noise, knocks, or changing noise level.],
  [Unlocking], [Visible, clean, and repeatable onset.], [Buried, intermittent, or varying onset.],
  [Final pulse], [Strong but not clipped.], [Flat top, clipping, or excessive resonance tail.],
  [tic and toc], [Both are detected consistently.], [One disappears or changes continuously.],
  [Trace], [Well-defined line without duplicate events.], [Clouds, jumps, or double triggering.],
  [Rate], [Stable over several measurement windows.], [Abrupt changes without a physical change in the watch.],
  [Amplitude], [Plausible and consistent over equivalent measurement windows.], [Large jumps while the rate remains stable.],
)

== Limits of Interpretation

The tg-expert waveform can be used to assess periodicity, stability, and detection quality. It can also be used to relate the beginning of the acoustic event generally to unlocking, the intermediate region to impulse, and the final pulse train to locking and banking. By itself, however, it cannot diagnose a specific component solely from an acoustic resonance or replace direct inspection of the escapement when the signal shows persistent abnormalities.

== References

1. Witschi Electronic AG. _Testing Methods for Mechanical Watches_. Büren an der Aare, 2025. Sections “Acoustic Measurement”, “The Beat Noise of the Swiss Lever Escapement”, “Rate Measurement”, “Beat Error”, and “Amplitude”.\
   #link("https://www.witschi.com/wp-content/uploads/firmwares/8Z1UtLGf/20251118_witschi_testing-methods-for-mechanical-watches_en.pdf")

2. Israel, Robin. “The Swiss lever escapement - Part 1: For everyone”. _R.I.O. Uhrenwerkstätte_, October 28, 2026.\
   #link("https://www.rio-uhren.de/en/blog/2026/10/28/schweizer-ankerhemmung/")

3. Velásquez Enriquez, Guido Alfonso. “The Beat Noise Of The Swiss Lever Escapement”. Video, October 21, 2013. Secondary source based, according to its description, on documentation from Witschi, Audemars Piguet, and timegrapher manuals.\
   #link("https://www.youtube.com/watch?v=_h6s_-78IwY")
