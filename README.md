# tg-expert

tg-expert is a desktop timegrapher for mechanical watches. It listens to the
ticking sound through the computer's audio input and shows rate, beat error,
amplitude and beat frequency in real time. It also records long-term trends
per watch position.

## Download

Windows installer (.msi), source tarball and checksums are on the
[releases page](https://github.com/jefebus/tg-expert/releases/latest).

Linux and macOS users build from source. See [BUILDING.md](BUILDING.md).

## Quick start

1. Install and start tg-expert. On first start, open the audio settings and
   choose the audio system and input device.
2. Put the watch close to the microphone, in a quiet room.
3. Select the watch's beat rate (bph) and lift angle, or let the program
   guess the bph.
4. Read the rate, beat error and amplitude from the results table and the
   paperstrip.

For accurate rate readings in seconds per day, calibrate the setup first.
The manual explains microphone placement, gain, noise and calibration.

## Manual

The user manual is built from `docs/manual/` with [Typst](https://typst.app)
and published as `tg-expert-manual-0.9.0.pdf` with each release.

## Credits and history

- **Tg** was written by Marcello Mamino and the contributors of
  [vacaboja/tg](https://github.com/vacaboja/tg).
- **tg-timer** is Alejandro Grigera's continuation of Tg.
- **tg-expert** continues tg-timer, adding new features as the audio settings dialog, a redesigned paperstrip and the Trend page. See [CHANGELOG.md](CHANGELOG.md).

Saved `.tgj` files from Tg and tg-timer open in tg-expert.

## License

tg-expert is distributed under the GNU General Public License, version 2.
See [LICENSE](LICENSE).
