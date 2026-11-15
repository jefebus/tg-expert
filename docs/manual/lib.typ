// Shared helpers for the tg-expert manual.

#let version = "0.9.0"

// Screenshot placeholder. Replace with
//   #figure(image("images/<file>"), caption: [...])
// once the screenshot exists in images/.
#let screenshot(file, caption) = figure(
  rect(width: 100%, height: 5cm, stroke: (dash: "dashed", paint: red), inset: 8pt)[
    #set align(center + horizon)
    #text(fill: red, weight: "bold")[SCREENSHOT NEEDED] \
    #text(fill: red)[images/#file] \
    #text(size: 9pt)[#caption]
  ],
  caption: caption,
)

// A screenshot from images/, scaled to the given share of the text width.
#let shot(file, caption, width: 100%) = figure(
  box(stroke: 0.5pt + luma(180), image("images/" + file, width: width)),
  caption: caption,
)

// A label or menu item as it appears in the program.
#let ui(body) = text(weight: "bold", body)

#let note(body) = block(
  fill: luma(240), inset: 8pt, radius: 3pt, width: 100%,
)[*Note.* #body]
