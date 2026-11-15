// tg-expert user manual.
// Build: typst compile docs/manual/main.typ tg-expert-manual-0.9.0.pdf
//
// Copyright (C) 2026 jefebus
// Distributed under the GNU General Public License, version 2.

#import "lib.typ": version

#set document(title: "tg-expert " + version + " User Manual", author: "jefebus")
#set page(paper: "a4", margin: (x: 2.2cm, y: 2.5cm), numbering: "1")
#set text(size: 10.5pt, lang: "en")
#set par(justify: true)
#set heading(numbering: "1.1")
#show heading.where(level: 1): it => { pagebreak(weak: true); it; v(0.4em) }
#show link: underline

#align(center + horizon)[
  #text(size: 28pt, weight: "bold")[tg-expert]
  #v(0.3em)
  #text(size: 14pt)[A software timegrapher for mechanical watches]
  #v(1.5em)
  #text(size: 12pt)[User manual, version #version]
]

#pagebreak()
#outline(depth: 2)

#include "chapters/introduction.typ"
#include "chapters/installation.typ"
#include "chapters/first-start.typ"
#include "chapters/microphone.typ"
#include "chapters/main-window.typ"
#include "chapters/measuring.typ"
#include "chapters/trend.typ"
#include "chapters/calibration.typ"
#include "chapters/files.typ"
#include "chapters/troubleshooting.typ"
#include "chapters/appendix.typ"
