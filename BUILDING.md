# Building tg-expert

## Dependencies

Runtime and build: GTK+ 3, GLib 2.0, PortAudio, FFTW3 (single precision,
`fftw3f`), pthread, libm. The PDF report also needs Cairo with its PDF
backend (`cairo-pdf`) and PangoCairo (`pangocairo`). Both come with GTK 3 in
every package list below; `configure` checks for them and stops if the PDF
backend is missing.

The report uses the DejaVu Sans font, like the manual, when it is
installed, and the system's sans-serif font otherwise. The Windows installer
does not include it.

Tools: a C99 compiler, autoconf, automake, libtool, make, pkg-config, git.

Translations: `msgfmt`, `msgmerge` and `xgettext` from GNU gettext (package
`gettext` in MSYS2, Debian, Fedora and Homebrew). Without them the program
builds in English only. The translations are in `po/`: after changing
translatable strings in the sources, run `make -C po update-po`, then edit
`po/<language>.po`. `po/GLOSSARY.md` lists the terms to use.

User manual: [Typst](https://typst.app). It's needed only to build the PDF
manual, which is also required for the Windows installer. Install it with
`winget install --id Typst.Typst` on Windows, `brew install typst` on macOS,
and on Linux from the distribution's packages where available, or as a binary
from the [Typst releases](https://github.com/typst/typst/releases).

Windows installer: the [WiX Toolset v3](https://wixtoolset.org) (`heat`,
`candle`, `light`) and `uuidgen`.

## Install the packages

### Windows (MSYS2 MINGW64)

Install [MSYS2](https://www.msys2.org), open the "MSYS2 MINGW64" shell and run:

```sh
pacman -S --needed mingw-w64-x86_64-gcc make pkg-config mingw-w64-x86_64-gtk3 \
  mingw-w64-x86_64-portaudio mingw-w64-x86_64-fftw git autoconf automake libtool
```

### Debian and Ubuntu

```sh
sudo apt-get install libgtk-3-dev libjack-jackd2-dev portaudio19-dev \
  libfftw3-dev git autoconf automake libtool
```

`libjack-jackd2-dev` works around an old Debian packaging bug.

### Fedora

```sh
sudo dnf install fftw-devel portaudio-devel gtk3-devel git autoconf automake libtool
```

### macOS (Homebrew)

```sh
brew install gtk+3 portaudio fftw pkg-config autoconf automake libtool
```

The macOS package list is unverified. The program needs microphone
permission for the terminal or app that starts it.

## Build

```sh
git clone https://github.com/jefebus/tg-expert.git
cd tg-expert
./autogen.sh
./configure
make
```

This produces `tg-expert` (`tg-expert.exe` on Windows). Other targets:

- `make tg-expert-dbg`: debug build.
- `make tg-expert-prf`: profiling build.
- `make tg-expert-vlg`, `make valgrind`: build and run under Valgrind.
- `make test`: build the test programs in `tests/` and the debug build.
- `make test-run`: build and run them all, then the built-in tests of the
  debug build (`./tg-expert-dbg test`). It names the ones that failed and
  exits with an error if any did.

## Test programs

`tests/` holds small programs, each with its own `main`. `make test` builds
them as `tests/<name>` and `make test-run` runs them from the build folder.
They print nothing when they pass, and `line N: ...` or an assertion when a
check fails. `tests/render_preview` writes `render-preview.pdf` and
`report-preview.pdf` in the folder it runs in. `tests/detection_status`
opens GTK, so it needs a desktop session. `tests/stubs.c` provides the few
functions of `interface.c` that the analysis code calls, and
`tests/stubs_output.c` those of `output_panel.c` that `correlations.c` calls.

A single test can also be built by hand, for example:

```sh
# Project metadata, notes, "Saved state" and date of snapshots in .tgj files
gcc -g -DPROGRAM_NAME='"Tg Expert"' -DVERSION='"test"' -o serializer_metadata \
    tests/serializer_metadata.c tests/stubs.c \
    src/algo.c src/algorithm_settings.c src/audio.c src/serializer.c src/trend.c \
    $(pkg-config --cflags --libs gtk+-3.0 portaudio-2.0 fftw3f) -lm -lpthread
./serializer_metadata

# Proposed file names and safe replacement of a file
gcc -g -DPROGRAM_NAME='"Tg Expert"' -DVERSION='"test"' -o session_file \
    tests/session_file.c $(pkg-config --cflags --libs gtk+-3.0)
./session_file

# The graphs in the three themes and a PDF report of synthetic data:
# writes render-preview.pdf and report-preview.pdf (add "mono" as a second
# argument for a monochrome report)
gcc -g -DPROGRAM_NAME='"Tg Expert"' -DVERSION='"test"' -o render_preview \
    tests/render_preview.c src/render.c src/correlations.c \
    src/trend.c src/trend_render.c src/waveform_render.c src/paperstrip_render.c \
    src/report.c src/session.c src/serializer.c \
    $(pkg-config --cflags --libs gtk+-3.0 cairo-pdf pangocairo) -lm
./render_preview render-preview.pdf
```

Write the `-D` options as above, each in its own quotes: kept in a shell
variable, the space in `"Tg Expert"` splits them.

To find a crash, run the debug build under gdb (`pacman -S
mingw-w64-x86_64-gdb` in MSYS2):

```sh
make tg-expert-dbg
gdb -batch -ex run -ex bt --args ./tg-expert-dbg
```
- `make install`: install the program, man page, desktop and MIME files.
- `make dist`: create the source tarball.

## User manual

The manual is written in Typst, in `docs/manual/`, in three languages:
`main.typ` with `chapters/` (English), `main-es.typ` with `es/` (Spanish) and
`main-de.typ` with `de/` (German). `lib.typ` and `images/` are shared. If
`typst` is in the PATH when `configure` runs:

- `make` also builds the manuals: `tg-expert-manual.pdf`,
  `tg-expert-manual-es.pdf` and `tg-expert-manual-de.pdf`;
- `make manual` builds only the manuals;
- `make install` installs them in the documentation folder. The
  "User manual" menu item opens the one in the language of the interface,
  or the English one.

Without Typst, `make manual` stops with a message. Run `./configure` again
after installing Typst.

To build the PDFs by hand, for example as release assets:

```sh
typst compile --root . docs/manual/main.typ    tg-expert-manual.pdf
typst compile --root . docs/manual/main-es.typ tg-expert-manual-es.pdf
typst compile --root . docs/manual/main-de.typ tg-expert-manual-de.pdf
```

When a chapter changes in English, update the same file in `es/` and `de/`.
The terms are in `po/GLOSSARY.md`, and the names of buttons and menus must
match the translations in `po/es.po` and `po/de.po`.

On Windows, winget installs Typst outside the MSYS2 PATH. Add its folder to
the PATH in the MINGW64 shell, or build the PDF from PowerShell.

## Windows installer

Build the installer in the MSYS2 MINGW64 shell, from the source folder:

1. Put `typst` in the PATH, so `make_msi.sh` builds the three manuals. Without
   it, the script uses `tg-expert-manual-0.10.0.pdf`, `-0.10.0-es.pdf` and
   `-0.10.0-de.pdf` from the source folder (see above), and stops if one is
   missing. `msgfmt` is needed too: the installer includes the translations.
2. Collect the runtime files to bundle (GTK, PortAudio and FFTW DLLs, and
   their data files):

   ```sh
   packaging/make_msi_resources.sh
   ```

   This copies the files listed in `packaging/tg-expert_resources` from
   `C:\msys64\mingw64` into `msi-resources/`. It expects MSYS2 in
   `C:\msys64`.
3. Build the MSI:

   ```sh
   packaging/make_msi.sh msi-resources
   ```

   This runs `configure` and `make`, then WiX. The result is
   `msi/tg-expert_0.9.0.msi`.
