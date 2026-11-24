// tg-expert user manual, German.
// Build: typst compile --root . docs/manual/main-de.typ tg-expert-manual-de.pdf
// Translation of main.typ and chapters/; terms in po/GLOSSARY.md.
//
// Copyright (C) 2026 jefebus
// Distributed under the GNU General Public License, version 2.

#import "lib.typ": manual

#show: manual.with(
  lang: "de",
  title: "Benutzerhandbuch",
  subtitle: "Eine Software-Zeitwaage für mechanische Uhren",
  edition: "Benutzerhandbuch, Version",
)

#include "de/introduction.typ"
#include "de/installation.typ"
#include "de/first-start.typ"
#include "de/microphone.typ"
#include "de/main-window.typ"
#include "de/measuring.typ"
#include "de/trend.typ"
#include "de/calibration.typ"
#include "de/files.typ"
#include "de/swiss_lever.typ"
#include "de/algorithm.typ"
#include "de/troubleshooting.typ"
#include "de/appendix.typ"
