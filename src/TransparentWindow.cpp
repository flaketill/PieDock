#include "TransparentWindow.h"

#include <stdlib.h>
#include <string.h>
#include <X11/Xutil.h>

#include <stdexcept>

using namespace PieDock;

/**
 * Initialize object
 *
 * @param a - Application object
 */
TransparentWindow::TransparentWindow(Application &a) :
		app(&a),
		width(app->getSettings()->getWidth()),
		height(app->getSettings()->getHeight()),
		canvas(0),
		buffer(0)
#ifdef HAVE_XRENDER
		,
		alphaPixmap(None),
		windowPicture(None),
		alphaPicture(None)
#endif
{
	Visual *visual = CopyFromParent;
	int depth = CopyFromParent;
	unsigned long vmask = 0;
	XSetWindowAttributes xswat;

	memset(&xswat, 0, sizeof(xswat));

	xswat.override_redirect = True;
	xswat.do_not_propagate_mask =
		KeyPressMask | KeyReleaseMask |
		ButtonPressMask | ButtonReleaseMask;
	vmask = CWOverrideRedirect | CWDontPropagate;

#ifdef HAVE_XRENDER
	if (app->getSettings()->useCompositing()) {
		// check if extensions for compositing are available
		{
			int majorOpcode;
			int firstEvent;
			int firstError;

			if (!XQueryExtension(
						app->getDisplay(),
						"RENDER",
						&majorOpcode,
						&firstEvent,
						&firstError) ||
					!XQueryExtension(
						app->getDisplay(),
						"Composite",
						&majorOpcode,
						&firstEvent,
						&firstError))
				throw std::runtime_error(
					"RENDER and/or Composite extension unavailable");
		}

		// find visual
		{
			XVisualInfo vi;

			// color depth must be ARGB for compositing
			depth = 32;

			if (!XMatchVisualInfo(
					app->getDisplay(),
					DefaultScreen(app->getDisplay()),
					depth,
					TrueColor,
					&vi)) {
				throw std::runtime_error(
					"cannot find a visual supporting alpha transparency");
			}

			visual = vi.visual;

			xswat.background_pixel = 0x00000000;
			xswat.border_pixel = 0x00000000;
			xswat.colormap = XCreateColormap(
				app->getDisplay(),
				DefaultRootWindow(app->getDisplay()),
				visual,
				AllocNone);

			vmask |= CWBackPixel | CWBorderPixel | CWColormap;
		}
	}
#endif

	if (!(window = XCreateWindow(app->getDisplay(),
			DefaultRootWindow(app->getDisplay()),
			0,
			0,
			width,
			height,
			0,
			depth,
			CopyFromParent,
			visual,
			vmask,
			&xswat))) {
		throw std::runtime_error("cannot create window");
	}

#ifdef HAVE_XRENDER
	if (app->getSettings()->useCompositing()) {
		if (!(alphaPixmap = XCreatePixmap(
					app->getDisplay(),
					window,
					width,
					height,
					32)) ||
				!(windowPicture = XRenderCreatePicture(
						app->getDisplay(),
						window,
						XRenderFindStandardFormat(
								app->getDisplay(),
								PictStandardARGB32),
						0,
						0)) ||
				!(alphaPicture = XRenderCreatePicture(
						app->getDisplay(),
						alphaPixmap,
						XRenderFindStandardFormat(
								app->getDisplay(),
								PictStandardARGB32),
						0,
						0))) {
			throw std::runtime_error("cannot create transparency pixmap");
		}
	} else
#endif
		// fall back to default values
	{
		int screen = DefaultScreen(app->getDisplay());

		visual = DefaultVisual(app->getDisplay(), screen);
		depth = DefaultDepth(app->getDisplay(), screen);
	}

	if (!(canvas = new XSurface(
			width,
			height,
			app->getDisplay(),
			visual,
			depth)) || !(buffer = new unsigned char[canvas->getSize()])) {
		throw std::runtime_error("cannot create offscreen surface");
	}

	if (!(gc = XCreateGC(
			app->getDisplay(),
			window,
			0,
			0))) {
		throw std::runtime_error("cannot create graphics context");
	}

	// you still need to select input
}

/**
 * Clean up
 */
TransparentWindow::~TransparentWindow() {
	// it's valid to delete 0
	delete canvas;
	delete buffer;

#ifdef HAVE_XRENDER
	if (alphaPixmap) {
		XFreePixmap(app->getDisplay(), alphaPixmap);
	}

	if (windowPicture) {
		XRenderFreePicture(app->getDisplay(), windowPicture);
	}

	if (alphaPicture) {
		XRenderFreePicture(app->getDisplay(), alphaPicture);
	}
#endif

	XDestroyWindow(app->getDisplay(), window);
}

/**
 * Show window; the caller must ensure that the window is completely
 * visible on the screen or XGetSubImage will fail !
 */
void TransparentWindow::show() {
	XMapRaised(app->getDisplay(), window);

#ifdef HAVE_XRENDER
	if (app->getSettings()->useCompositing()) {
		clear();
		return;
	}
#endif

	XGetSubImage(
		app->getDisplay(),
		window,
		0,
		0,
		width,
		height,
		0xffffffff,
		ZPixmap,
		canvas->getResource(),
		0,
		0);

	memcpy(
		buffer,
		canvas->getData(),
		canvas->getSize());
}

/**
 * Hide window
 */
void TransparentWindow::hide() const {
	XUnmapWindow(app->getDisplay(), window);
}

/**
 * Clear window for drawing
 */
void TransparentWindow::clear() {
#ifdef HAVE_XRENDER
	if (app->getSettings()->useCompositing()) {
		memset(
			canvas->getData(),
			0,
			canvas->getSize());

		return;
	}
#endif

	memcpy(
		canvas->getData(),
		buffer,
		canvas->getSize());
}

/**
 * Update window
 */
void TransparentWindow::update() const {
#ifdef HAVE_XRENDER
	if (app->getSettings()->useCompositing()) {
		XPutImage(
			app->getDisplay(),
			window,
			gc,
			canvas->getResource(),
			0,
			0,
			0,
			0,
			width,
			height);

		XPutImage(
			app->getDisplay(),
			alphaPixmap,
			gc,
			canvas->getResource(),
			0,
			0,
			0,
			0,
			width,
			height);

		composite();

		return;
	}
#endif

	XPutImage(
		app->getDisplay(),
		window,
		gc,
		canvas->getResource(),
		0,
		0,
		0,
		0,
		width,
		height);
}
