// tg-expert user manual, English.
// Build: typst compile --root . docs/manual/main.typ tg-expert-manual.pdf
// Other languages: main-es.typ (chapters in es/), main-de.typ (chapters in de/).
//
// Copyright (C) 2026 jefebus
// Distributed under the GNU General Public License, version 2.

#import "lib.typ": manual

#show: manual.with(
  lang: "en",
  title: "User Manual",
  subtitle: "A software timegrapher for mechanical watches",
  edition: "User manual, version",
)

#include "chapters/introduction.typ"
#include "chapters/installation.typ"
#include "chapters/first-start.typ"
#include "chapters/microphone.typ"
#include "chapters/main-window.typ"
#include "chapters/measuring.typ"
#include "chapters/trend.typ"
#include "chapters/calibration.typ"
#include "chapters/files.typ"
#include "chapters/swiss_lever.typ"
#include "chapters/algorithm.typ"
#include "chapters/troubleshooting.typ"
#include "chapters/appendix.typ"
