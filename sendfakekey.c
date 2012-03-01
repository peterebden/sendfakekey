/* 
    sendfakekey

    This is a small application to send fake key events to X, in the
    spirit of xsendkey. It leans on libfakekey to do keyboard remapping
    if the keycode doesn't already exist (in cases where the key is not
    on the keyboard, for example). libfakekey doesn't seem to deliver the
    events to the same place though since they go via a different mechanism
    so I am trying to sort of combine both.

    Note that there is no support for modifier keys (yet?).
    
    Copyright (c) 2012 Peter Ebden <peter.ebden@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <fakekey/fakekey.h>

Window window;

int error_handler(Display* display, XErrorEvent* event) {
    fprintf(stderr, "XSendEvent(0x%lx) failed.\n", window);
    return 1;
}


int main(int argc, char* argv[]) {
    if(argc != 2) {
	fprintf(stderr, "Usage: sendfakekey <keysym>\n"
		        "Keysyms can be found in /usr/include/X11/keysymdef.h\n"
  		        "with the XK_ prefix removed.\n");
	return EXIT_FAILURE;
    }

    Display* display = XOpenDisplay(0);
    if(display == NULL) {
        fprintf(stderr, "Couldn't connect to X display\n");
        return EXIT_FAILURE;
    }

    KeySym ks = XStringToKeysym(argv[1]);
    if(ks == NoSymbol) {
        fprintf(stderr, "Failed to create a KeySym from given string %s.\n", argv[1]);
        return EXIT_FAILURE;
    }

    KeyCode kc = XKeysymToKeycode(display, ks);
    if(kc == 0) {
	// Failed to create a keycode from the keysym. Try to get libfakekey to do tricks for us.	
	FakeKey* fk = fakekey_init(display);
	if(!fk) {
	    fprintf(stderr, "Failed to create FakeKey structure.\n");
	    return EXIT_FAILURE;
	}
	if(!fakekey_press_keysym(fk, ks, 0)) {
	    fprintf(stderr, "Keysym press failed\n");
	}
	fakekey_release(fk);
	// try again on the keycode now libfakekey should have remapped it.
	kc = XKeysymToKeycode(display, ks);
	if(kc == 0) {
	    fprintf(stderr, "Failed to create a KeyCode from given KeySym %d\n", (int)ks);
	    return EXIT_FAILURE;
	}
    }

    int unused;
    XGetInputFocus(display, &window, &unused);

    // Now send the actual key event
    XKeyEvent		event;
    event.display	= display;
    event.window        = window;
    event.root		= RootWindow(display, 0);
    event.subwindow	= None;
    event.time		= CurrentTime;
    event.x		= 1;
    event.y		= 1;
    event.x_root	= 1;
    event.y_root	= 1;
    event.same_screen	= True;
    event.type		= KeyPress;
    event.state		= 0;
    event.keycode       = kc;

    XSync(display, False);
    XSetErrorHandler(error_handler);
    XSendEvent(display, event.window, True, KeyPressMask, (XEvent*)&event);
    XSync(display, False); // unsure if this is really necessary, but mirrors what was done in xsendevent.
    event.type = KeyRelease;
    XSendEvent(display, event.window, True, KeyPressMask, (XEvent*)&event);
    XSync(display, False);
    XSetErrorHandler(NULL);

    XCloseDisplay(display);
    return 0;
}
