// tg-expert user manual, Spanish.
// Build: typst compile docs/manual/main-es.typ tg-expert-manual-es.pdf
// Translation of main.typ and chapters/; terms in po/GLOSSARY.md.
//
// Copyright (C) 2026 jefebus
// Distributed under the GNU General Public License, version 2.

#import "lib.typ": manual

#show: manual.with(
  lang: "es",
  title: "Manual de usuario",
  subtitle: "Un cronocomparador por software para relojes mecánicos",
  edition: "Manual de usuario, versión",
)

#pagebreak(weak: true)
#include "es/introduction.typ"
#include "es/installation.typ"
#include "es/first-start.typ"
#include "es/microphone.typ"
#include "es/main-window.typ"
#include "es/measuring.typ"
#include "es/trend.typ"
#include "es/calibration.typ"
#include "es/files.typ"
/*#include "es/swiss_lever.typ"*/
#include "es/troubleshooting.typ"
#include "es/appendix.typ"
