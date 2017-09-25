#include "XSurface.h"

#include <string.h>
#include <stdlib.h>
#include <X11/Xutil.h>

#include <stdexcept>

using namespace PieDock;

/**
 * Initialize surface
 *
 * @param w - width of image in pixels
 * @param h - height of image in pixels
 * @param d - X Display
 * @param v - visual structure for XImage (optional)
 * @param depth - desired depth of XImage (optional)
 */
XSurface::XSurface(int w, int h, Display *d, Visual *v, int depth) :
		display(d),
		visual(v),
		orginalDepth(depth),
		resource(0) {
	calculateSize(w, h, determineBitsPerPixel(depth));
	allocateData();
}

/**
 * Clean up
 */
XSurface::~XSurface() {
	freeData();
}

/**
 * Determine how many bits are used by the given color depth; this depends
 * on X which tries to align the pixels on a multiple of 4 for speed;
 * this means 24 bit RGB is actually 32 bits per pixel
 *
 * @param depth - color depth
 */
int XSurface::determineBitsPerPixel(int depth) {
	XPixmapFormatValues *pf;
	int formats;
	int bitsPerPixel = depth;

	pf = XListPixmapFormats(display, &formats);

	if (formats) {
		int format;

		for (format = formats; format--;) {
			if (pf[format].depth == bitsPerPixel) {
				bitsPerPixel = pf[format].bits_per_pixel;
				break;
			}
		}

		XFree(reinterpret_cast<char *>(pf));
	}

	return bitsPerPixel;
}

/**
 * Allocate data
 */
void XSurface::allocateData() {
	setData(reinterpret_cast<unsigned char *>(calloc(
				getSize(),
				sizeof(char))));

	if (!getData()) {
		throw std::runtime_error("cannot allocate memory");
	}

	if (!(resource = XCreateImage(
			display,
			visual,
			orginalDepth,
			ZPixmap,
			0,
			reinterpret_cast<char *>(getData()),
			getWidth(),
			getHeight(),
			32,
			0))) {
		throw std::runtime_error("cannot create XImage");
	}
}

/**
 * Free data
 */
void XSurface::freeData() {
	XDestroyImage(resource);
	resource = 0;
	// data is freed by XDestroyImage
	setData(0);
}
