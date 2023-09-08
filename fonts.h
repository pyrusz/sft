#ifndef FONTS_H
#define FONTS_H

#include <fontconfig/fontconfig.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H

#include <xcb/render.h>
#include <xcb/xcb_renderutil.h>

#include <alloca.h>

#include "khash.h"
KHASH_MAP_INIT_INT(int_map, uint32_t) /* hash map with uint32_t keys and values */


enum {
	SUCCESS,
	FAILURE
};

struct font_context {
	xcb_connection_t	*x_connection;
	FT_Library	ft_library;
	double	dpi;
	xcb_render_pictformat_t	a8_format;
	xcb_render_pictformat_t	screen_format;
};

struct font {
	FT_Face	ft_face;
	xcb_render_glyphset_t	glyph_set;
	khash_t(int_map)	*character_map; /* maps character code to glyph index */
};

/* Corresponds to GLYPHELT32 */
struct glyph_command {
	uint8_t	count;
	_Alignas(4) int16_t	x;
	int16_t	y;
	uint32_t	glyphs[];
};


int find_visual_format(xcb_render_query_pict_formats_reply_t *formats_reply, xcb_visualid_t visual, xcb_render_pictformat_t *format);
int fonts_init(xcb_connection_t *connection, struct font_context *font_context);
int load_font(struct font_context *font_context, char *search_string, struct font *font);
int render_string(struct font_context *font_context, struct font *font, xcb_render_picture_t source, xcb_render_picture_t destination, int x, int y, char *string);
int fonts_end(struct font_context *font_context);





#endif
