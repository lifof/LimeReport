/* maxipng.h - Shapes for Maxicode output to PNG file */

/*
    libzint - the open source barcode library
    Copyright (C) 2008 Robin Stuart <robin@zint.org.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/* This file contains the pixel-by-pixel representation of maxicode glyphs
   at a resolution of 12 pixels per millimeter. hexagon[] is taken directly
   from ISO 16023 Annex J. bullseye[] was calculated by the Gimp */

#define SSET	"0123456789ABCDEF"

static int hexagon[120] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 1, 1, 1, 0, 0, 0,
	0, 0, 0, 1, 1, 1, 1, 1, 0, 0,
	0, 0, 1, 1, 1, 1, 1, 1, 1, 0,
	0, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 1, 1, 1, 1, 1, 1, 1, 0,
	0, 0, 0, 1, 1, 1, 1, 1, 0, 0,
	0, 0, 0, 0, 1, 1, 1, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static unsigned int bullseye_compressed[] = {
	0,0,0,0,0,255,248,0,0,0,0,0,
	0,0,0,0,31,255,255,192,0,0,0,0,
	0,0,0,1,255,255,255,252,0,0,0,0,
	0,0,0,7,255,255,255,255,0,0,0,0,
	0,0,0,31,255,255,255,255,192,0,0,0,
	0,0,0,127,255,255,255,255,240,0,0,0,
	0,0,1,255,255,255,255,255,252,0,0,0,
	0,0,7,255,255,255,255,255,255,0,0,0,
	0,0,15,255,255,0,7,255,255,128,0,0,
	0,0,63,255,240,0,0,127,255,224,0,0,
	0,0,127,255,128,0,0,15,255,240,0,0,
	0,0,255,252,0,0,0,1,255,248,0,0,
	0,1,255,240,0,0,0,0,127,252,0,0,
	0,3,255,224,0,0,0,0,63,254,0,0,
	0,7,255,128,0,0,0,0,15,255,0,0,
	0,15,255,0,0,0,0,0,7,255,128,0,
	0,31,252,0,0,127,240,0,1,255,192,0,
	0,63,248,0,7,255,255,0,0,255,224,0,
	0,127,240,0,63,255,255,224,0,127,240,0,
	0,127,224,0,255,255,255,248,0,63,240,0,
	0,255,192,1,255,255,255,252,0,31,248,0,
	1,255,128,7,255,255,255,255,0,15,252,0,
	1,255,0,15,255,255,255,255,128,7,252,0,
	3,255,0,63,255,255,255,255,224,7,254,0,
	3,254,0,127,255,192,31,255,240,3,254,0,
	7,252,0,255,252,0,1,255,248,1,255,0,
	7,252,1,255,240,0,0,127,252,1,255,0,
	15,248,1,255,192,0,0,31,252,0,255,128,
	15,240,3,255,128,0,0,15,254,0,127,128,
	31,240,7,255,0,0,0,7,255,0,127,192,
	31,224,7,254,0,0,0,3,255,0,63,192,
	63,224,15,252,0,0,0,1,255,128,63,224,
	63,224,31,248,0,63,192,0,255,192,63,224,
	63,192,31,240,0,255,240,0,127,192,31,224,
	63,192,63,224,3,255,252,0,63,224,31,224,
	127,192,63,224,7,255,254,0,63,224,31,240,
	127,128,63,192,15,255,255,0,31,224,15,240,
	127,128,127,192,31,255,255,128,31,240,15,240,
	127,128,127,128,63,255,255,192,15,240,15,240,
	127,128,127,128,63,255,255,192,15,240,15,240,
	255,0,127,128,127,240,255,224,15,240,7,240,
	255,0,255,128,127,192,63,224,15,248,7,240,
	255,0,255,0,255,128,31,240,7,248,7,240,
	255,0,255,0,255,128,31,240,7,248,7,240,
	255,0,255,0,255,0,15,240,7,248,7,240,
	255,0,255,0,255,0,15,240,7,248,7,240,
	255,0,255,0,255,0,15,240,7,248,7,240,
	255,0,255,0,255,0,15,240,7,248,7,240,
	255,0,255,0,255,128,31,240,7,248,7,240,
	255,0,255,0,255,128,31,240,7,248,7,240,
	255,0,255,0,127,192,63,224,7,248,7,240,
	255,0,255,128,127,240,255,224,15,248,7,240,
	255,0,127,128,63,255,255,192,15,240,7,240,
	127,128,127,128,63,255,255,192,15,240,15,240,
	127,128,127,128,31,255,255,128,15,240,15,240,
	127,128,127,192,15,255,255,0,31,240,15,240,
	127,128,63,192,7,255,254,0,31,224,15,240,
	127,192,63,224,3,255,252,0,63,224,31,240,
	63,192,63,224,0,255,240,0,63,224,31,224,
	63,192,31,240,0,63,192,0,127,192,31,224,
	63,224,31,248,0,0,0,0,255,192,63,224,
	63,224,15,252,0,0,0,1,255,128,63,224,
	31,224,7,254,0,0,0,3,255,0,63,192,
	31,240,7,255,0,0,0,7,255,0,127,192,
	15,240,3,255,128,0,0,15,254,0,127,128,
	15,248,1,255,192,0,0,31,252,0,255,128,
	7,252,1,255,240,0,0,127,252,1,255,0,
	7,252,0,255,252,0,1,255,248,1,255,0,
	3,254,0,127,255,192,31,255,240,3,254,0,
	3,255,0,63,255,255,255,255,224,7,254,0,
	1,255,0,15,255,255,255,255,128,7,252,0,
	1,255,128,7,255,255,255,255,0,15,252,0,
	0,255,192,1,255,255,255,252,0,31,248,0,
	0,127,224,0,255,255,255,248,0,63,240,0,
	0,127,240,0,63,255,255,224,0,127,240,0,
	0,63,248,0,7,255,255,0,0,255,224,0,
	0,31,252,0,0,127,240,0,1,255,192,0,
	0,15,255,0,0,0,0,0,7,255,128,0,
	0,7,255,128,0,0,0,0,15,255,0,0,
	0,3,255,224,0,0,0,0,63,254,0,0,
	0,1,255,240,0,0,0,0,127,252,0,0,
	0,0,255,252,0,0,0,1,255,248,0,0,
	0,0,127,255,128,0,0,15,255,240,0,0,
	0,0,63,255,240,0,0,127,255,224,0,0,
	0,0,15,255,255,0,7,255,255,128,0,0,
	0,0,7,255,255,255,255,255,255,0,0,0,
	0,0,1,255,255,255,255,255,252,0,0,0,
	0,0,0,127,255,255,255,255,240,0,0,0,
	0,0,0,31,255,255,255,255,192,0,0,0,
	0,0,0,7,255,255,255,255,0,0,0,0,
	0,0,0,1,255,255,255,252,0,0,0,0,
	0,0,0,0,31,255,255,192,0,0,0,0,
	0,0,0,0,0,255,248,0,0,0,0,0
};
