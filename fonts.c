#include "fonts.h"

int find_a8_format(xcb_render_query_pict_formats_reply_t *formats_reply, xcb_render_pictformat_t *format) {
	for (xcb_render_pictforminfo_iterator_t i = xcb_render_query_pict_formats_formats_iterator(formats_reply);
			i.rem;
			xcb_render_pictforminfo_next(&i)) {
		if (i.data->type != XCB_RENDER_PICT_TYPE_DIRECT)
			continue;
		if (i.data->depth != 8)
			continue;
		if (i.data->direct.red_mask != 0)
			continue;
		if (i.data->direct.green_mask != 0)
			continue;
		if (i.data->direct.blue_mask != 0)
			continue;
		if (i.data->direct.alpha_mask != 0xff)
			continue;
		*format = i.data->id;
		return SUCCESS;
	}

	return FAILURE;
}

int find_visual_format(xcb_render_query_pict_formats_reply_t *formats_reply, xcb_visualid_t visual, xcb_render_pictformat_t *format) {
	xcb_render_pictscreen_iterator_t screen_iterator;
	xcb_render_pictdepth_iterator_t depth_iterator;
	xcb_render_pictvisual_iterator_t visual_iterator;

	for (screen_iterator = xcb_render_query_pict_formats_screens_iterator(formats_reply);
			screen_iterator.rem;
			xcb_render_pictscreen_next(&screen_iterator)) {
		for (depth_iterator = xcb_render_pictscreen_depths_iterator(screen_iterator.data);
				depth_iterator.rem;
				xcb_render_pictdepth_next(&depth_iterator)) {
			for (visual_iterator = xcb_render_pictdepth_visuals_iterator(depth_iterator.data);
					visual_iterator.rem;
					xcb_render_pictvisual_next(&visual_iterator)) {
				if (visual_iterator.data->visual == visual) {
					*format = visual_iterator.data->format;
					return SUCCESS;
				}
			}
		}
	}

	return FAILURE;
}

// continue
xcb_render_picture_t xcbft_create_pen(xcb_connection_t *c, xcb_render_color_t color)
{
	xcb_render_pictforminfo_t *fmt;
	const xcb_render_query_pict_formats_reply_t *fmt_rep =
		xcb_render_util_query_formats(c);
	// alpha can only be used with a picture containing a pixmap
	fmt = xcb_render_util_find_standard_format(
		fmt_rep,
		XCB_PICT_STANDARD_ARGB_32
	);

	xcb_drawable_t root = xcb_setup_roots_iterator(
			xcb_get_setup(c)
		).data->root;

	xcb_pixmap_t pm = xcb_generate_id(c);
	xcb_create_pixmap(c, 32, pm, root, 1, 1);

	uint32_t values[1];
	values[0] = XCB_RENDER_REPEAT_NORMAL;

	xcb_render_picture_t picture = xcb_generate_id(c);

	xcb_render_create_picture(c,
		picture,
		pm,
		fmt->id,
		XCB_RENDER_CP_REPEAT,
		values);

	xcb_rectangle_t rect = {
		.x = 0,
		.y = 0,
		.width = 1,
		.height = 1
	};

	xcb_render_fill_rectangles(c,
		XCB_RENDER_PICT_OP_OVER,
		picture,
		color, 1, &rect);

	xcb_free_pixmap(c, pm);
	return picture;
}


int fonts_init(xcb_connection_t *x_connection, struct font_context *font_context) {
	if (FcInit() == FcFalse) {
		fputs("Could not initialize Fontconfig\n", stderr);
		return FAILURE;
	}

	if (FT_Init_FreeType(&font_context->ft_library) != FT_Err_Ok) {
		fputs("Could not initialize Freetype\n", stderr);
		return FAILURE;
	}

	font_context->x_connection = x_connection;
	font_context->dpi = 111.0;

	xcb_render_query_pict_formats_cookie_t formats_cookie = xcb_render_query_pict_formats(x_connection);
	xcb_generic_error_t *error = NULL;
	xcb_render_query_pict_formats_reply_t *formats_reply = xcb_render_query_pict_formats_reply(x_connection, formats_cookie, &error);
	if (!formats_reply) {
		fprintf(stderr, "X error: %d\n", error->error_code);
		free(error);
		return FAILURE;
	}

	if (find_a8_format(formats_reply, &font_context->a8_format) == FAILURE) {
		fputs("Could not find a8 picture format\n", stderr);
		return FAILURE;
	}

	return SUCCESS;
}

int fonts_end(struct font_context *font_context) {
	if (FT_Done_FreeType(font_context->ft_library) != FT_Err_Ok) {
		fputs("Could not delete Freetype library object\n", stderr);
		return FAILURE;
	}

	return SUCCESS;
}


/*
	if (find_visual_format(formats_reply, screen->root_visual, &font_context->a8_format) == FAILURE) {
		fputs("Could not find default picture format\n", stderr);
		return FAILURE;
	}
*/


int load_font(struct font_context *font_context, char *search_string, struct font *font) {
	int return_status = FAILURE;

	FcPattern *search_pattern = FcNameParse((FcChar8*) search_string);
	if (search_pattern == NULL) {
		fputs("Could not build font search pattern\n", stderr);
		return FAILURE;
	}

	if (FcConfigSubstitute(NULL, search_pattern, FcMatchPattern) == FcFalse) {
		fprintf(stderr, "Could not execute font config substitutions");
	}

	FcPatternIter iterator;
	if (FcPatternFindIter(search_pattern, &iterator, FC_DPI) == 0)
		FcPatternAddDouble(search_pattern, FC_DPI, font_context->dpi);

	FcDefaultSubstitute(search_pattern);

	FcResult result;
	FcPattern *result_pattern = FcFontMatch(NULL, search_pattern, &result);
	if (result_pattern == NULL) {
		fprintf(stderr, "No match found for font pattern");
		goto out;
	}

	FcChar8 *file_path = NULL; /* FcChar8 is unsigned char */
	if (FcPatternGetString(result_pattern, FC_FILE, 0, &file_path) != FcResultMatch) {
		fputs("Could not get file path from font pattern\n", stderr);
		goto out;
	}

	int font_index = 0; /* If font has no index, use 0 */
	FcPatternGetInteger(result_pattern, FC_INDEX, 0, &font_index);

	double font_size = 0;
	if (FcPatternGetDouble(result_pattern, FC_SIZE, 0, &font_size) != FcResultMatch) {
		fputs("Could not get font size from font pattern\n", stderr);
		goto out;
	}

	double dpi = 0;
	if (FcPatternGetDouble(result_pattern, FC_DPI, 0, &dpi) != FcResultMatch) {
		fputs("Could not get dpi from font pattern\n", stderr);
		goto out;
	}

	if (FT_New_Face(font_context->ft_library, (char *) file_path, font_index, &font->ft_face) != FT_Err_Ok) {
		fprintf(stderr, "Could not open font '%s'\n", (char *) file_path);
		goto out;
	}

	if (FT_Set_Char_Size(font->ft_face, 0, font_size*64, dpi, 0) != FT_Err_Ok) {
		fputs("Could not set font size (ppem)\n", stderr);
		goto out;
	}

	if (FT_Select_Charmap(font->ft_face, FT_ENCODING_UNICODE) != FT_Err_Ok) {
		fputs("Could not load character map of font\n", stderr);
		return FAILURE;
	}

	font->glyph_set = xcb_generate_id(font_context->x_connection);
	xcb_render_create_glyph_set(font_context->x_connection, font->glyph_set, font_context->a8_format);

	font->character_map = kh_init(int_map);

	return_status = SUCCESS;
	out:
		FcPatternDestroy(search_pattern);
		FcPatternDestroy(result_pattern);
		return return_status;
}

int load_glyph(xcb_connection_t *x_connection, FT_Face ft_face, xcb_render_glyphset_t glyph_set, uint32_t glyph_index) {
	if (FT_Load_Glyph(ft_face, glyph_index, FT_LOAD_RENDER) != FT_Err_Ok) {
		fputs("Could not render glyph\n", stderr);
		return FAILURE;
	}

	FT_Bitmap *ft_bitmap = &ft_face->glyph->bitmap;
	xcb_render_glyphinfo_t xcb_glyph;
	xcb_glyph.x = -ft_face->glyph->bitmap_left;
	xcb_glyph.y = ft_face->glyph->bitmap_top;
	xcb_glyph.width = ft_bitmap->width;
	xcb_glyph.height = ft_bitmap->rows;
	xcb_glyph.x_off = ft_face->glyph->advance.x/64;
	xcb_glyph.y_off = ft_face->glyph->advance.y/64;

	int xcb_stride = (xcb_glyph.width + 3) & ~3;
	int xcb_size = xcb_stride*xcb_glyph.height;

	// TODO: use stack
	uint8_t *xcb_bitmap = calloc(sizeof (uint8_t), xcb_size);
	if (xcb_bitmap == NULL) {
		perror("calloc");
		return FAILURE;
	}

	for (int i = 0; i < xcb_glyph.height; ++i)
		memcpy(xcb_bitmap + i*xcb_stride, ft_bitmap->buffer + i*xcb_glyph.width, xcb_glyph.width);

	xcb_render_add_glyphs(x_connection, glyph_set, 1, &glyph_index, &xcb_glyph, xcb_size, xcb_bitmap);
	free(xcb_bitmap);
	return SUCCESS;
}

void render_glyphs(xcb_connection_t *x_connection, xcb_render_picture_t source, xcb_render_picture_t destination, int x, int y, xcb_render_glyphset_t glyph_set, int glyph_count, uint32_t *glyphs) {
	//xcb_render_glyph_t *glyphs (= u32)

	/* TODO: process in batches */
	if (glyph_count > 254)
		glyph_count = 254;

	int glyphs_size = glyph_count * sizeof (uint32_t);
	int glyph_command_size = sizeof (struct glyph_command) + glyphs_size;
	struct glyph_command *glyph_command = alloca(glyph_command_size);

	//struct glyph_command glyph_command_header = {glyph_count, {0, 0, 0}, x, y};
	struct glyph_command glyph_command_header = {glyph_count, x, y};

	memcpy(glyph_command, &glyph_command_header, sizeof glyph_command_header);
	memcpy(glyph_command->glyphs, glyphs, glyphs_size);

	xcb_render_composite_glyphs_32(x_connection, XCB_RENDER_PICT_OP_OVER, source, destination, XCB_NONE, glyph_set, 0, 0, glyph_command_size, (uint8_t *) glyph_command);
}

int render_string(struct font_context *font_context, struct font *font, xcb_render_picture_t source, xcb_render_picture_t destination, int x, int y, char *string) {
	/* loop to process each utf8 character */
	int i = 0;
	int remaining_length = strlen(string);
	FcChar8 *current_position = (FcChar8 *) string;
	FcChar32 ucs4;
	uint32_t glyph_indexes[remaining_length];

	while (remaining_length) {
		int char_length = FcUtf8ToUcs4(current_position, &ucs4, remaining_length);
		if (char_length <= 0) {
			fprintf(stderr, "Could not convert from UTF8 to UCS4 (1st byte: 0x%x)\n", *current_position);
			return FAILURE;
		}

		/* try to locate character in hash map */
		khint_t hash_map_slot = kh_get(int_map, font->character_map, (uint32_t) ucs4);

		if (hash_map_slot != kh_end(font->character_map)) { /* character found */
			glyph_indexes[i] = kh_value(font->character_map, hash_map_slot);
		} else { /* character not found, add it */
			glyph_indexes[i] = (uint32_t) FT_Get_Char_Index(font->ft_face, ucs4); /* Uses 0 as fall-back index */

			int return_code;
			hash_map_slot = kh_put(int_map, font->character_map, ucs4, &return_code);
			if (return_code == -1) {
				fputs("Could not add key to hash map\n", stderr);
				return FAILURE;
			}
			kh_value(font->character_map, hash_map_slot) = glyph_indexes[i];

			if (load_glyph(font_context->x_connection, font->ft_face, font->glyph_set, glyph_indexes[i]) == FAILURE) {
				fputs("Could not load glyph\n", stderr);
				return FAILURE;
			}
		}

		++i;
		remaining_length -= char_length;
		current_position += char_length;
	}

	render_glyphs(font_context->x_connection, source, destination, x, y, font->glyph_set, i, glyph_indexes);

	return SUCCESS;
}
