// Shared helpers for the tg-expert manual, in every language.
// Paths in these functions (images/) are relative to this file.
// Presentation adapted from Guia_rapida_tg-expert_es.typ.

#let version = sys.inputs.at("version", default: read("../../version").trim())

#let navy = rgb("17324D")
#let teal = rgb("147D83")
#let pale = rgb("EAF3F4")
#let orange = rgb("FFF3D8")
#let ink = rgb("243746")
#let grid-color = rgb("C8D5DA")

#let manual(lang: "en", title: "User Manual", subtitle: "", edition: "", body) = {
  set document(title: "tg-expert " + version + " " + title, author: "jefebus")
  set page(
    paper: "a4",
    margin: (x: 18mm, y: 17mm),
    footer: context [
      #line(length: 100%, stroke: 0.5pt + luma(210))
      #v(3pt)
      #text(size: 8pt, fill: luma(90))[tg-expert #version · #title]
      #h(1fr)
      #text(size: 8pt, fill: luma(90))[#counter(page).display("1")]
    ],
  )
  set text(font: "DejaVu Sans", size: 9.3pt, fill: ink, lang: lang)
  set par(
    leading: 1.10em,
    spacing: 0.8em,
    justify: false,
  )
  set heading(numbering: "1.1")

  // Keep the original heading element, numbering, labels and outline entries.
  show heading.where(level: 1): it => {
    pagebreak(weak: true)
    set text(size: 16pt, weight: "bold", fill: navy)
    it
    v(0.4em)
  }
  show heading.where(level: 2): set text(size: 11pt, weight: "bold", fill: teal)
  show heading.where(level: 2): set block(above: 1.8em, below: 0.6em, sticky: true)
  show heading.where(level: 3): set text(size: 10pt, weight: "bold", fill: teal)
  show heading.where(level: 3): set block(above: 1.3em, below: 0.4em, sticky: true)
  show link: underline

  set table(
    inset: 5pt,
    stroke: 0.35pt + grid-color,
    fill: (x, y) => if y == 0 { navy }
      else if calc.rem(y, 2) == 0 { rgb("F5F8F9") }
      else { white },
  )
  show table.cell.where(y: 0): set text(fill: white, weight: "bold")
  set figure(gap: 5pt)
  show figure.caption: set text(size: 8pt, fill: luma(90))
  show figure: set block(below: 1.8em)

  align(center + horizon)[
    #text(size: 28pt, weight: "bold", fill: navy)[tg-expert]
    #v(0.3em)
    #text(size: 16pt, fill: navy)[#title]
    #v(0.5em)
    #text(size: 11pt, fill: luma(80), subtitle)
    #v(1.5em)
    #text(size: 9pt, fill: teal)[#edition #version]
  ]
  pagebreak()
  outline(depth: 2)
  body
}

// Existing placeholder helper; production images still use shot().
#let screenshot(file, caption) = figure(
  rect(width: 100%, height: 5cm, stroke: (dash: "dashed", paint: teal), inset: 8pt)[
    #set align(center + horizon)
    #text(fill: teal, weight: "bold")[SCREENSHOT NEEDED] \
    #text(fill: teal)[images/#file] \
    #text(size: 9pt)[#caption]
  ],
  caption: caption,
)

#let shot(file, caption, width: 100%) = figure(
  box(stroke: 0.5pt + grid-color, image("images/" + file, width: width)),
  caption: caption,
)

#let ui(body) = text(weight: "bold", fill: navy, body)

// Existing translated labels, unchanged from the original shared library.
// Note box: optional background and label in the document language.
#let note(body, color: pale) = {
  let labels = (
    en: "Note.",
    es: "Nota.",
    de: "Hinweis.",
  )

  block(
    fill: pale,
    stroke: 0.5pt + teal,
    inset: 8pt,
    radius: 2pt,
    width: 100%,
  )[
    #context strong(labels.at(text.lang, default: "Note."))
    #body
  ]
}