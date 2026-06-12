/*
 * Generates an Xcursor theme at runtime: the regular arrow pointer, drawn
 * slightly smaller, inside a translucent gold halo with a yellow ring. The
 * compositor renders it as the hardware cursor, so the highlight tracks the
 * pointer with zero latency.
 */
#include <cairo/cairo.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "cursor.h"

#define CURSOR_PI 3.14159265358979323846

static const int cursor_sizes[] = { 24, 32, 48, 64, 96 };
#define N_SIZES (sizeof(cursor_sizes) / sizeof(cursor_sizes[0]))

static void draw_cursor(cairo_t *cr, int s) {
	double c = s / 2.0;
	double ring = s / 14.0 < 2.0 ? 2.0 : s / 14.0;
	double r = c - ring;

	/* halo: translucent gold fill, dark outline for light backgrounds,
	 * yellow ring */
	cairo_arc(cr, c, c, r, 0, 2 * CURSOR_PI);
	cairo_set_source_rgba(cr, 1.0, 0.84, 0.0, 0.20);
	cairo_fill_preserve(cr);
	cairo_set_line_width(cr, ring + 2.0);
	cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.50);
	cairo_stroke_preserve(cr);
	cairo_set_line_width(cr, ring);
	cairo_set_source_rgba(cr, 1.0, 0.84, 0.0, 0.95);
	cairo_stroke(cr);

	/* small arrow with its tip on the hotspot (the halo center) */
	double h = s * 0.40;
	cairo_move_to(cr, c, c);
	cairo_line_to(cr, c, c + h * 0.72);
	cairo_line_to(cr, c + h * 0.17, c + h * 0.55);
	cairo_line_to(cr, c + h * 0.30, c + h * 0.80);
	cairo_line_to(cr, c + h * 0.38, c + h * 0.74);
	cairo_line_to(cr, c + h * 0.25, c + h * 0.51);
	cairo_line_to(cr, c + h * 0.48, c + h * 0.51);
	cairo_close_path(cr);
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
	cairo_fill_preserve(cr);
	cairo_set_line_width(cr, s / 32.0 < 1.0 ? 1.0 : s / 32.0);
	cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
	cairo_stroke(cr);
}

static int write_u32(FILE *f, uint32_t v) {
	unsigned char b[4] = {
		v & 0xff, (v >> 8) & 0xff, (v >> 16) & 0xff, (v >> 24) & 0xff,
	};
	return fwrite(b, 1, 4, f) == 4 ? 0 : -1;
}

static int write_image(FILE *f, int s) {
	cairo_surface_t *surf =
		cairo_image_surface_create(CAIRO_FORMAT_ARGB32, s, s);
	cairo_t *cr = cairo_create(surf);
	draw_cursor(cr, s);
	cairo_destroy(cr);
	cairo_surface_flush(surf);

	int ret = 0;
	/* image chunk: header, type, nominal size, version, w, h, hotspot,
	 * delay, then premultiplied ARGB pixels (cairo's native layout) */
	uint32_t hdr[] = { 36, 0xfffd0002, s, 1, s, s, s / 2, s / 2, 0 };
	for (size_t i = 0; i < sizeof(hdr) / sizeof(hdr[0]); ++i) {
		ret |= write_u32(f, hdr[i]);
	}
	unsigned char *data = cairo_image_surface_get_data(surf);
	int stride = cairo_image_surface_get_stride(surf);
	for (int y = 0; y < s; ++y) {
		for (int x = 0; x < s; ++x) {
			uint32_t px;
			memcpy(&px, data + y * stride + x * 4, 4);
			ret |= write_u32(f, px);
		}
	}
	cairo_surface_destroy(surf);
	return ret;
}

static int write_xcursor(const char *path) {
	FILE *f = fopen(path, "w");
	if (!f) {
		return -1;
	}
	int ret = 0;
	ret |= write_u32(f, 0x72756358); /* "Xcur" */
	ret |= write_u32(f, 16);
	ret |= write_u32(f, 0x10000);
	ret |= write_u32(f, N_SIZES);
	uint32_t pos = 16 + N_SIZES * 12;
	for (size_t i = 0; i < N_SIZES; ++i) {
		ret |= write_u32(f, 0xfffd0002);
		ret |= write_u32(f, cursor_sizes[i]);
		ret |= write_u32(f, pos);
		pos += 36 + cursor_sizes[i] * cursor_sizes[i] * 4;
	}
	for (size_t i = 0; i < N_SIZES; ++i) {
		ret |= write_image(f, cursor_sizes[i]);
	}
	if (fclose(f) != 0) {
		ret = -1;
	}
	return ret;
}

static int mkdir_p(char *path) {
	for (char *p = path + 1; *p; ++p) {
		if (*p != '/') {
			continue;
		}
		*p = '\0';
		if (mkdir(path, 0755) != 0 && errno != EEXIST) {
			*p = '/';
			return -1;
		}
		*p = '/';
	}
	if (mkdir(path, 0755) != 0 && errno != EEXIST) {
		return -1;
	}
	return 0;
}

int cursor_theme_install(void) {
	char theme_dir[512];
	const char *data = getenv("XDG_DATA_HOME");
	const char *home = getenv("HOME");
	int n;
	if (data && data[0]) {
		n = snprintf(theme_dir, sizeof(theme_dir),
				"%s/icons/wshowkeys-cursor", data);
	} else if (home && home[0]) {
		n = snprintf(theme_dir, sizeof(theme_dir),
				"%s/.local/share/icons/wshowkeys-cursor", home);
	} else {
		return -1;
	}
	if (n < 0 || (size_t)n >= sizeof(theme_dir) - 16) {
		return -1;
	}

	char path[600];
	snprintf(path, sizeof(path), "%s/cursors", theme_dir);
	if (mkdir_p(path) != 0) {
		return -1;
	}

	/* Inherit the previous theme so every other cursor shape (text beam,
	 * resize arrows, ...) is unaffected. */
	const char *inherits = getenv("XCURSOR_THEME");
	if (!inherits || !inherits[0]
			|| strcmp(inherits, "wshowkeys-cursor") == 0) {
		inherits = "default";
	}
	snprintf(path, sizeof(path), "%s/index.theme", theme_dir);
	FILE *f = fopen(path, "w");
	if (!f) {
		return -1;
	}
	fprintf(f, "[Icon Theme]\nName=wshowkeys-cursor\nInherits=%s\n",
			inherits);
	if (fclose(f) != 0) {
		return -1;
	}

	snprintf(path, sizeof(path), "%s/cursors/left_ptr", theme_dir);
	if (write_xcursor(path) != 0) {
		return -1;
	}

	static const char *aliases[] = { "default", "arrow" };
	for (size_t i = 0; i < sizeof(aliases) / sizeof(aliases[0]); ++i) {
		snprintf(path, sizeof(path), "%s/cursors/%s", theme_dir, aliases[i]);
		unlink(path);
		if (symlink("left_ptr", path) != 0) {
			return -1;
		}
	}
	return 0;
}
