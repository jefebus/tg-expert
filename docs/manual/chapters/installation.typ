#import "../lib.typ": *

= Installation

== Windows

Download `tg-expert_0.9.0.msi` from the
#link("https://github.com/jefebus/tg-expert/releases/latest")[releases page]
and run it. The installer puts tg-expert in its own folder under
`Program Files`, adds Start menu entries for the program and for this manual,
and associates `.tgj` files with the program.

On every system, #ui[User manual] in the menu opens this manual. If the
manual isn't installed, it opens the releases page, where the PDF can be
downloaded.

tg-expert installs next to tg-timer and does not replace it. The two
programs keep separate settings.

To uninstall, use *Settings > Apps* in Windows.

#note[Windows may ask whether to allow the program to use the microphone. If
the microphone was denied, see @troubleshooting.]

== Linux

There is no binary package for Linux yet. Build from source as described in
`BUILDING.md` in the source tarball, then run `make install`. This installs
the program, the man page, a desktop entry and the `.tgj` file type, and this
manual if Typst was available during the build.

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
