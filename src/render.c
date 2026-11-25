/*
    tg-expert
    Copyright (C) 2026 jefebus

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

/* TG-31: drawing themes shared by the screen and the PDF report.
 *
 * The graph renderers take a cairo context, a size in device units (pixels
 * on screen, points in a PDF), their data and options, and one of these
 * themes. They never look at GTK widgets, so the same code draws the screen
 * and the document.
 *   - screen: the colours on black that the program always had;
 *   - document colour: on white, darker colours that print well;
 *   - document monochrome: black and greys; positions differ by symbol. */

#include "tg.h"
#include <pango/pangocairo.h>

static const struct render_theme themes[RENDER_THEME_COUNT] = {
	[RENDER_SCREEN] = {
		.id = RENDER_SCREEN,
		.monochrome = 0,
		.background = {0, 0, 0},
		.text = {1, 1, 1},
		.label = {.85, .85, .85},
		.message = {.75, .75, .75},
		.grid = {1, 1, 1},
		.grid_alpha = .22,
		.grid_minor_alpha = .16,
		.line_scale = 1,
		.symbol_scale = 1,
		.position = {
			{.65, .65, .65},	// not set
			{.30, .65, 1},		// DU
			{1, .31, .82},		// DD
			{.98, .76, .02},	// CU
			{.35, .85, .45},	// CD
			{.71, .49, 1},		// CL
			{1, .48, .25}		// CR
		},
		// The agreed screen colours (see the palette in output_panel.c)
		.axis = {1, 1, 1},
		.axis_major_alpha = .55,
		.axis_minor_alpha = .25,
		.rate = {.302, .651, 1},
		.amp = {1, .310, .816},
		.be = {.710, .486, 1},
		.tic = {.722, .722, .722},
		.toc = {.980, .761, .020},
		.wave = {.902, .902, .902}
	},
	[RENDER_DOC_COLOR] = {
		.id = RENDER_DOC_COLOR,
		.monochrome = 0,
		.background = {1, 1, 1},
		.text = {0, 0, 0},
		.label = {.2, .2, .2},
		.message = {.35, .35, .35},
		.grid = {0, 0, 0},
		.grid_alpha = .35,
		.grid_minor_alpha = .18,
		// Paper is read closer than a screen: smaller symbols and finer lines,
		// so that dense readings stay separate points instead of a band
		.line_scale = .6,
		.symbol_scale = .6,
		.position = {
			{.45, .45, .45},
			{.05, .40, .80},
			{.80, .10, .60},
			{.70, .50, 0},
			{.10, .55, .20},
			{.45, .25, .80},
			{.85, .40, .05}
		},
		// The same meanings as on screen, darker for white paper
		.axis = {0, 0, 0},
		.axis_major_alpha = .55,
		.axis_minor_alpha = .30,
		.rate = {.05, .35, .75},
		.amp = {.75, .05, .55},
		.be = {.45, .20, .75},
		.tic = {.45, .45, .45},
		.toc = {.70, .50, 0},
		.wave = {.10, .10, .10}
	},
	[RENDER_DOC_MONO] = {
		.id = RENDER_DOC_MONO,
		.monochrome = 1,
		.background = {1, 1, 1},
		.text = {0, 0, 0},
		.label = {.2, .2, .2},
		.message = {.35, .35, .35},
		.grid = {0, 0, 0},
		.grid_alpha = .35,
		.grid_minor_alpha = .18,
		.line_scale = .6,
		.symbol_scale = .6,
		.position = {
			{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
			{0, 0, 0}, {0, 0, 0}, {0, 0, 0}
		},
		// Each series has its own graph, so black is enough; tic and toc
		// differ by grey level
		.axis = {0, 0, 0},
		.axis_major_alpha = .55,
		.axis_minor_alpha = .30,
		.rate = {0, 0, 0},
		.amp = {0, 0, 0},
		.be = {0, 0, 0},
		.tic = {.55, .55, .55},
		.toc = {.25, .25, .25},
		.wave = {0, 0, 0}
	}
};

const struct render_theme *render_theme_get(enum render_theme_id id)
{
	if(id < 0 || id >= RENDER_THEME_COUNT) id = RENDER_SCREEN;
	return &themes[id];
}

void render_set_rgb(cairo_t *cr, struct render_rgb c, double alpha)
{
	cairo_set_source_rgba(cr, c.r, c.g, c.b, alpha);
}

/* The symbol of a watch position, the same in the graphs and in their
 * legends, so that a printed legend matches the points exactly. Positions
 * differ by shape, so they stay apart in monochrome too. */
void render_position_symbol(cairo_t *cr, const struct render_theme *th, int position,
	double x, double y, double radius)
{
	if(position < POS_NONE || position >= POS_COUNT) position = POS_NONE;
	radius *= th->symbol_scale;
	// A text drawn before leaves a current point: without this the circle of
	// DU would be joined to it by a line
	cairo_new_path(cr);
	render_set_rgb(cr, th->position[position], 1);
	// The stroke follows the size, so that outlined and filled symbols
	// keep the same weight
	cairo_set_line_width(cr, 1.4 * radius / 3.5);
	switch(position) {
	case POS_NONE:
		cairo_move_to(cr, x, y - radius);
		cairo_line_to(cr, x + radius, y);
		cairo_line_to(cr, x, y + radius);
		cairo_line_to(cr, x - radius, y);
		cairo_close_path(cr);
		cairo_stroke(cr);
		break;
	case POS_DU:
		cairo_arc(cr, x, y, radius, 0, 2 * M_PI);
		cairo_fill(cr);
		break;
	case POS_DD:
		cairo_rectangle(cr, x - radius, y - radius, 2 * radius, 2 * radius);
		cairo_fill(cr);
		break;
	case POS_CU:
	case POS_CD: {
		int sign = position == POS_CU ? 1 : -1;
		cairo_move_to(cr, x, y - sign * radius);
		cairo_line_to(cr, x + radius, y + sign * radius);
		cairo_line_to(cr, x - radius, y + sign * radius);
		cairo_close_path(cr);
		cairo_fill(cr);
		break;
	}
	case POS_CL:
		cairo_move_to(cr, x - radius, y);
		cairo_line_to(cr, x + radius, y);
		cairo_move_to(cr, x, y - radius);
		cairo_line_to(cr, x, y + radius);
		cairo_stroke(cr);
		break;
	case POS_CR:
		cairo_move_to(cr, x - radius, y - radius);
		cairo_line_to(cr, x + radius, y + radius);
		cairo_move_to(cr, x - radius, y + radius);
		cairo_line_to(cr, x + radius, y - radius);
		cairo_stroke(cr);
		break;
	default:
		break;
	}
}

/* Legend of the position symbols on one line from (x, y) (baseline), with
 * the same symbols as the points. Only the positions in mask (bit p for
 * position p) are listed; 0 lists all. Returns the x after the last item. */
double render_position_legend(cairo_t *cr, const struct render_theme *th, double x, double y,
	double font, unsigned mask)
{
	static const char *codes[POS_COUNT] = {NULL, "DU", "DD", "CU", "CD", "CL", "CR"};
	cairo_set_font_size(cr, font);
	for(int pos = POS_NONE; pos < POS_COUNT; pos++) {
		if(mask && !(mask & (1u << pos))) continue;
		render_position_symbol(cr, th, pos, x + font * 0.35, y - font * 0.35, font * 0.35);
		/* TRANSLATORS: legend of the graphs: readings taken while no watch
		 * position was set. Short */
		const char *text = codes[pos] ? codes[pos] : _("not set");
		render_set_rgb(cr, th->text, 1);
		render_text(cr, x + font, y, 0, text);
		cairo_text_extents_t e;
		cairo_text_extents(cr, text, &e);
		x += font + e.x_advance + font * 1.2;
	}
	return x;
}

/* A short centred message, wrapped, for a graph without data */
void render_message(cairo_t *cr, const struct render_theme *th, double width, double height,
	const char *message)
{
	PangoLayout *layout = pango_cairo_create_layout(cr);
	if(th->id != RENDER_SCREEN) {
		// Documents are measured in points: sizes as given, not at 96 dpi
		pango_cairo_context_set_resolution(pango_layout_get_context(layout), 72);
		pango_layout_context_changed(layout);
	}
	pango_layout_set_text(layout, message, -1);
	pango_layout_set_width(layout, (int)((width - 20) * PANGO_SCALE));
	pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
	pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
	int text_height;
	pango_layout_get_pixel_size(layout, NULL, &text_height);
	render_set_rgb(cr, th->message, 1);
	cairo_move_to(cr, 10, (height - text_height) / 2.0);
	pango_cairo_show_layout(cr, layout);
	g_object_unref(layout);
}

/* Text at (x, y) on its baseline: align 0 = left, .5 = centre, 1 = right */
void render_text(cairo_t *cr, double x, double y, double align, const char *text)
{
	cairo_text_extents_t extents;
	cairo_text_extents(cr, text, &extents);
	cairo_move_to(cr, x - align * extents.x_advance, y);
	cairo_show_text(cr, text);
}
