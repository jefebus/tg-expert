#import "../lib.typ": *

= The measurement algorithm <algorithm>

TG-Expert uses the escapement's sound to measure timekeeping and balance
amplitude. It compares several beats to distinguish tic from toc.
Their sounds must repeat clearly enough, so good microphone contact
often helps more than changing algorithm settings.

== How the measurement is obtained

=== The signal used for measurement

Before measuring, the program filters slow vibration and forms an envelope
that brings out each beat's sounds. Full mode also removes unusually
energetic passages to reduce interference. Noise suppression cannot itself
distinguish interference from a very strong tick: if genuine beats disappear,
check its threshold.

Detection compares the signal with time-shifted copies to find regular
repetition. A louder recording does not necessarily give a better measurement.

=== Beat rate and timekeeping

The measured period is a complete *tic-to-tic* cycle: *7200/BPH seconds*.
The average interval between adjacent beats is half that, *3600/BPH seconds*.
At 14,400 BPH, for example, the cycle lasts 500 ms, with an average 250 ms
between tic and toc.

If you know the movement's BPH, select it so the program searches near that
period. Auto estimates the rhythm and matches it to the nearest BPH preset.
Check an unexpected result against the watch's specifications: even a stable
estimate can represent the wrong rhythm. An alternative candidate shown in
manual mode does not change your selected BPH.

RATE shows how much the watch gains or loses relative to that nominal
rhythm. It also depends on the sound card's clock accuracy; @calibration
explains how to correct this time base.

Full mode uses 2, 4, 8 and 16 second blocks; Light uses 1, 2, 4 and 8.
Results are selected for stability. After changing settings, let the blocks
refill: a previous reading marked as old is not a fresh measurement.

=== Waveforms and beat error

The waveforms bring many cycles together in one image. The highest values
at each position are discarded before averaging to reduce isolated impacts.
The display represents a typical sound, not a recording of one individual tick.

Toc is located near the middle of that cycle. Unequal tic-to-toc and
toc-to-tic intervals produce beat error. Widening this search may help with
genuinely uneven beats, but does not correct a steady gain or loss.

=== Amplitude and paperstrip dots

Amplitude is estimated from acoustic lift time and the selected lift angle,
which must suit the movement. The detector checks that tic and toc estimates
exceed the minimum, stay below 360° and do not disagree too much.
Even so, a plausible number does not prove that the right pulses were identified.

Detected lift times also refine B.E. and the timing of paperstrip dots.
Without amplitude, preliminary beat error can still be displayed; without
tic/toc separation, the current implementation does not update RATE.
Dots are found individually, so some may be missing even with a recognized
rhythm, particularly before enough audio is available around a beat.

== Full and Light modes

Full is the usual choice and the default. Light reduces processing load
and gives earlier readings, but is generally less stable and more sensitive
to noise: it processes half the samples and skips interference suppression.

Light grays out the three noise-suppression controls without deleting their
values or help. Returning to Full enables them again. The rest of detection
still works in both modes; BPH and lift angle remain unchanged.

== Configuration parameters <algorithm-configuration>

#note(color: orange)[*For experienced users:* change algorithm parameters only
if you understand what they control and how they affect detection. Otherwise,
leave them at their defaults. Changing settings by trial and error can produce
incorrect readings that still look plausible.]

=== Dialog, levels and applying changes

Open #ui[Algorithm configuration…] from the menu to find these settings.
#ui[Basic] lets you choose Light mode; #ui[Advanced] shows the five more
commonly useful movement controls. To investigate a particular stage,
#ui[Show Expert settings] adds thirteen more under the same headings.
Changing which controls are shown does not change or apply their values.

You can fold each group and open any parameter's #ui[Help] with mouse or
keyboard. Values that differ from their defaults are highlighted for easy
review. BPH, lift angle and sample rate remain in the usual movement and
audio controls. #ui[Calibration] has its own tab, described in
@calibration-parameters.

- #ui[Apply] applies changes and saves them, leaving the dialog open.
- #ui[Save] also applies and saves, then closes the dialog.
- #ui[Reset to defaults] resets movement controls and Light in the dialog.
  It resets calibration controls only while calibration editing is enabled.
  The reset is not applied until you choose Apply or Save.
- #ui[Cancel] discards unapplied edits, but cannot undo an earlier Apply.

Save a snapshot before applying if you want to compare with the current
measurement: changes restart analysis and clear the live Trend. Filters
and buffers are then rebuilt with the new values. If the configuration
cannot be saved, an error is shown and the changes are not applied.

=== Units, ranges and stored values

Use the tables to check the original value and the available adjustment
range. Pay particular attention to *± ms*: 20 means 20 ms on each side,
not a 20 ms search in total. Without ±, milliseconds describe a window or
segment length. *×* is a multiplier; each percentage refers to the quantity
being compared. Amplitude is in degrees, while calibration also uses seconds
and s/day. All numeric values use a decimal point.

A value within its range is not always enough: after conversion to samples,
the windows must fit the available audio. The program checks that at least
one noise reference segment remains, that there is room for smoothing, and
that calibration positions are ordered within the middle of the folded second.

The INI file stores changes under `[algorithm]`, using the table's keys.
Returning a parameter to its default removes that entry, leaving only
customizations. Valid older settings and unknown keys are retained.
An invalid older value produces a warning and measurement uses the default.
Saving movement settings does not overwrite locked, unedited calibration values.

=== Advanced movement controls

#table(
  columns: (2.8fr, 1.2fr, 0.65fr, 0.95fr),
  [Parameter], [INI key], [Default], [Range],
  [Audio and envelope cutoff (Hz)], [`filter`], [3000], [500–5000],
  [Noise rejection threshold (× reference)], [`noise_threshold`], [2], [1–3],
  [Period search half-width (± ms)], [`guess_search`], [20], [10–30],
  [Opposite-beat search half-width (± ms)], [`be_search`], [20], [10–30],
  [Minimum accepted balance amplitude (°)], [`amp_min`], [135], [90–135],
)

/ Audio and envelope cutoff:
  If the recording picks up slow vibration, a higher cutoff can help reject
  it while retaining finer envelope detail. The same value controls the
  audio high-pass and envelope low-pass; it is not a volume control.
  Use Apply to rebuild the filters with the new cutoff.
/ Noise rejection threshold:
  Full mode mutes passages whose energy exceeds this multiple of the
  reference. If strong genuine ticks disappear, try raising it:
  *higher values mute less*. Lowering it strengthens rejection.
  There is no independent Off setting, and it has no effect in Light mode.
/ Period search half-width:
  Allows for a watch whose period differs from nominal. In manual mode,
  the center is 7200/BPH seconds: at 14,400 BPH and ±20 ms, the search
  covers 480–520 ms, not 230–270. Widen it if the real cycle falls outside
  that window. It is also used when checking automatic candidates.
/ Opposite-beat search half-width:
  Toc is sought near the middle of the measured cycle. More room may help
  with genuinely uneven tic/toc timing, but also allows an unrelated peak
  to be chosen and distort beat error. A steady gain or loss is not a reason
  to widen this search.
/ Minimum accepted balance amplitude:
  You can lower this for a genuinely low-amplitude movement. Both tic and
  toc estimates must exceed it; lowering it does not help identify the
  pulses more accurately. The tic/toc views expand leftward when the
  selected minimum requires it, and the full-period highlights follow.
  That expansion changes the display, not the evidence for a correct detection.

=== Expert movement controls

#table(
  columns: (2.8fr, 1.2fr, 0.65fr, 0.95fr),
  [Parameter], [INI key], [Default], [Range],
  [Noise energy window (ms)], [`noise_window`], [20], [10–30],
  [Noise reference segment length (ms)], [`noise_segment`], [500], [300–700],
  [Period refinement half-width (± ms)], [`period_refine`], [20], [10–30],
  [Minimum shorter-period peak (%)], [`harmonic`], [90], [80–100],
  [Double-trigger check threshold (%)], [`double_trigger`], [20], [10–30],
  [Tic/toc alignment smoothing (ms)], [`tic_smoothing`], [0.5], [0.1–1],
  [High samples discarded per phase bin (%)], [`trim`], [20], [10–30],
  [Trace event search half-width (± ms)], [`event_search`], [20], [10–30],
  [Lift-pulse smoothing (ms)], [`amp_smoothing`], [1], [0.5–2],
  [Starting lift-pulse threshold (% of peak)], [`amp_initial`], [1], [0.1–5],
  [Lift-pulse background multiplier (×)], [`amp_noise`], [1.4], [1.2–2],
  [Lift-pulse threshold ceiling (% of peak)], [`amp_limit`], [20], [10–30],
  [Maximum tic/toc amplitude difference (°)], [`amp_difference`], [60], [60–100],
)

/ Noise energy window:
  This is how much audio goes into each local energy measurement. A longer
  window lets a short impact influence more time. Consider it alongside
  the noise threshold when examining which passages are muted; it operates
  only in Full mode.
/ Noise reference segment length:
  The reference is not average microphone volume. Each segment contributes
  its highest local energy, then the median of those maxima is taken.
  Longer segments leave fewer reference measurements in each block.
  Like the other noise controls, it has no effect in Light mode.
/ Period refinement half-width:
  Once the period is found, the program refines it by seeking later correlation
  peaks near their predicted positions. This margin belongs to that second
  search. Widening it makes sense if the correct peak is outside; otherwise
  it may allow a neighboring peak to be chosen.
/ Minimum shorter-period peak:
  Makes the choice of a shorter automatic period more or less demanding.
  Its correlation peak must exceed this percentage of the original peak.
  Raising the percentage lets fewer shorter candidates through.
  These are correlation peaks, not the volume of microphone sounds.
/ Double-trigger check threshold:
  Helps decide whether to try a doubled period. Correlation near 1.5 times
  the candidate period is compared with correlation at that period;
  a ratio below the threshold requests the doubled-period check.
  Raising it makes that check more likely, rather than setting another
  volume threshold. This control and the preceding one also affect
  alternative estimates in manual mode, but not its main search around
  the selected BPH.
/ Tic/toc alignment smoothing:
  The combined tic/toc envelope is smoothed before the timing reference
  is chosen. More smoothing removes fine detail but can move the alignment
  point. A cleaner-looking image alone is not a reason to increase it.
/ High samples discarded per phase bin:
  When building the representative waveform, the highest samples at each
  position are removed before averaging cycles. Raising the percentage
  reduces isolated impacts, but also gives unusually loud beats less
  weight. The discarded count is rounded up; even a small bin retains
  at least one sample.
/ Trace event search half-width:
  If dots are missing, more room around each predicted beat may recover
  them. It may also select another acoustic feature from the same tick.
  This setting affects individual paperstrip dots, not the initial
  period search.
/ Lift-pulse smoothing:
  The detector looks for pulses in a smoothed waveform. More smoothing
  may remove ripples, but can also merge nearby features and make them
  harder to identify.
/ Starting lift-pulse threshold:
  This is the search's starting point, expressed as a percentage of the
  strongest smoothed waveform peak. It is not always the threshold used:
  if the background-based calculation gives a higher value, the search
  starts there instead.
/ Lift-pulse background multiplier:
  The estimated background is multiplied by this value to obtain a
  detection threshold. Raising it makes weak features harder to accept
  as lift pulses; it does not increase the recorded signal.
/ Lift-pulse threshold ceiling:
  The search tries increasing thresholds until it reaches this percentage
  of the peak. A higher ceiling allows trials at higher thresholds, not
  a higher maximum amplitude. If the starting threshold already reaches
  the ceiling, no trials at a lower threshold are made.
/ Maximum tic/toc amplitude difference:
  For the pair to be accepted, the difference between its amplitudes must
  be less than this limit. Raising it lets them disagree more; it does
  not make them more accurate. Even a pair that passes can come from
  incorrectly identified pulses.

== Try one change at a time

1. Start with defaults, correct BPH and lift angle, and clean audio. Improve
  microphone contact before relaxing detection. Save a snapshot for comparison.
2. Identify the failing stage and change one relevant control. Apply, let
  the blocks refill, then compare stable readings—not just the first plausible number.
3. Keep the change only if it helps; otherwise reset and apply again.
  Calibration resets require its edit checkbox to be enabled.

For rate-only work, leave lift-pulse controls at defaults. For a genuinely
low-amplitude movement, try the Advanced minimum first, rather than several
pulse thresholds. Acceptance is not proof of accuracy.
