#ifndef _PieDock_WindowManager_
#define _PieDock_WindowManager_

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <string>
#include <vector>
#include <map>

static int ignoreHandler(Display *, XErrorEvent *) {
	return True;
}

namespace PieDock {
// forward declaration
class ArgbSurface;

class WindowManager {
public:
	class WindowList : public std::vector<Window> {
	public:
		WindowList(Display *d) {
			addClientsOf(d);
		}
		virtual ~WindowList() {}
		void addClientsOf(Display *);
	};

	virtual ~WindowManager() {}
	static void activate(Display *, Window);
	static void iconify(Display *, Window);
	static void close(Display *, Window);
	static Window getActive(Display *);
	static Window getClientWindow(Display *, Window);
	static std::string getTitle(Display *, Window);
	static ArgbSurface *getIcon(Display *, Window);
	static unsigned long getWorkspace(Display *, Window);
	static unsigned long getNumberOfWorkspaces(Display *);
	static unsigned long getCurrentWorkspace(Display *);
	static bool getWorkspacePosition(Display *, unsigned long &, unsigned long &);
	static bool getDesktopGeometry(Display *, unsigned long &, unsigned long &);
	static bool isNormalWindow(Display *, Window);
	static void setWindowType(Display *, Window, const char *);
	static void sendClientMessage(Display *, Window, const char *,
		unsigned long = 0, unsigned long = 0, unsigned long = 0,
		unsigned long = 0, unsigned long = 0);
	static Atom getAtom(Display *, const char *);

private:
	template <class T> class Property {
	public:
		Property(Display *d, Window w) :
			display(d),
			window(w),
			data(0),
			items(0) {}
		virtual ~Property() {
			freeData();
		}
		inline T *getData() const {
			return data;
		}
		inline unsigned long getItems() const {
			return items;
		}
		bool fetch(Atom type, const char *name,
				long length = 1024, long offset = 0,
				Bool remove = False) {
			freeData();

			Atom returnedType;
			int format;
			unsigned long items;
			unsigned long bytesAfter;
			unsigned char *data;

			XErrorHandler defaultHandler =
				XSetErrorHandler(ignoreHandler);

			if (XGetWindowProperty(
					display,
					window,
					getAtom(display, name),
					offset,
					length,
					remove,
					type,
					&returnedType,
					&format,
					&items,
					&bytesAfter,
					&data) != Success) {
				return false;
			}

			XSetErrorHandler(defaultHandler);

			if (returnedType != type) {
				freeData();
				return false;
			}

			this->data = reinterpret_cast<T *>(data);
			this->items = items;

			return true;
		}

	private:
		Display *display;
		Window window;
		T *data;
		unsigned long items;

		void freeData() {
			if (!data) {
				return;
			}

			XFree(reinterpret_cast<unsigned char *>(data));

			data = 0;
			items = 0;
		}
	};

	typedef std::map<std::string, Atom> StringToAtom;
	static StringToAtom stringToAtom;

	WindowManager() {}
	WindowManager &operator=(const WindowManager &) {
		return *this;
	}
};
}

#endif
