#import "../lib.typ": *

= Installation

== Windows

Download the Windows `.msi` installer from the latest release on the
#link("https://github.com/jefebus/tg-expert/releases")[releases page]
and run it. The installer puts tg-expert in its own folder under
`Program Files`, adds Start menu entries for the program and for this manual,
and associates `.tgj` files with the program.

On every system, #ui[User manual] in the menu opens this manual. If the
manual isn't installed, it opens the releases page, where the PDF can be
downloaded.

To uninstall, use *Settings > Apps* in Windows.

#note[Windows may ask whether to allow the program to use the microphone. If
the microphone was denied, see @troubleshooting.]

== Linux

A `.deb` package for Debian and Ubuntu systems on x86-64 (amd64) is available in the
latest release on the
#link("https://github.com/jefebus/tg-expert/releases")[releases page].
Download and install it using your package manager.

Alternatively, build from source as described in `BUILDING.md` in the
source tarball, then run `make install`. This installs the program, the man
page, a desktop entry and the `.tgj` file type, and this manual if Typst was
available during the build.

== macOS

There is no macOS package. Building from source with Homebrew should work,
following `BUILDING.md`, but it has not been verified for this release. On
first start, macOS asks for microphone permission for the program or for the
terminal it was started from. Allow it in *System Settings > Privacy &
Security > Microphone*.

== Settings file

tg-expert keeps its settings in `tg-expert.ini`, in the user configuration
folder (`%LOCALAPPDATA%` on Windows, `~/.config` on Linux). It stores the
window size, the audio settings, the cal value, the saved watches and the
paperstrip settings. Deleting it resets the program to its defaults.

== Language

The program shows its texts in the language of the operating system:
English, Spanish or German. For any other language it uses English.
#ui[User manual] in the menu opens the manual in the same language.

Some things stay the same in every language, so that readings can be
compared with any timegrapher:

- numbers always use a point as the decimal separator, as in +2.5 s/d;
- the results table headers RATE, AMP., B.E., L.A. and BEAT, and its units;
- the position codes DU, DD, CU, CD, CL and CR;
- the calibration status: cal (wait), cal (acq.), cal (done), cal (fail).

On Linux, the language can be chosen when starting the program, for
example `LANGUAGE=es tg-expert`.
