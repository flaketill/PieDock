#ifndef _PieDock_Application_
#define _PieDock_Application_

#include <X11/Xlib.h>
#include <string>

#include "Settings.h"

namespace PieDock {
class Application {
public:
	Application(Settings &);
	virtual ~Application();
	inline Display *getDisplay() const {
		return display;
	}
	inline Settings *getSettings() {
		return settings;
	}

	bool remote(const char * = 0) const;
	int run(bool *);

private:
	static const char StopMarker;
	static const char *Show;

	enum PulseBeats {
		StandBy = 0,
		Active = 10000
	};

	enum {
		UnixPathMax = 108
	};

	Display *display;
	Window root;
	Settings *settings;
	int suspend;
	std::string socketFile;

	void grabTriggers();
	void ungrabTriggers();
};
}

#endif
