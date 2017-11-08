#include <cstdlib>
#include <iostream>
#include <vector>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <unistd.h>
#include <list>
#include <string>
#include <sys/time.h>
#include <math.h>
#include <sstream>

using namespace std;

#include "simon.h"
/* Information to draw on the window. */
struct XInfo {
    Display* 	display;
    int		 screen;
    Window	 window;
    GC gc;
};

int fps = 60;
enum Colour {BLACK, WHITE};
XInfo xinfo;
int clicked_counter = 0;
int clicked_x;
int clicked_y;
int frame_counter = 30;
int score = 0;
string message = "Press SPACE to play";
bool lock = false;
int delay = 0;

// an array of graphics contexts to demo
GC  gc[2];

char* itoa(int val, int base){
    if (val == 0){
        return "0";
    }
    static char buf[32] = {0};
    int i = 30;
    for(; val && i ; --i, val /= base)
        buf[i] = "0123456789abcdef"[val % base];
    return &buf[i+1];
}

int stoi(string s) {
    stringstream geek(s);
    int x = 0;
    geek >> x;
    return x;
}

void setForeground(Colour c) {
    if (c == BLACK) {
        XSetForeground(xinfo.display, xinfo.gc, BlackPixel(xinfo.display, xinfo.screen));
    } else {
        XSetForeground(xinfo.display, xinfo.gc, WhitePixel(xinfo.display, xinfo.screen));
    }
}

// get microseconds
unsigned long now() {
    timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}


/* Function to put out a message on error exits. */
void error( string str ) {
    cerr << str << endl;
    exit(0);
}

// An abstract class representing displayable things.
class Displayable {
public:
    virtual void paint(XInfo& xinfo) = 0;
    virtual void mouseover(int, int) = 0;
    virtual void mouseclick(int, int) = 0;
    virtual string getType() = 0;
    virtual int getOrder() = 0;
    virtual int getX()=0;
    virtual int getY()=0;
    virtual string getStr() = 0;
    virtual void changebool(bool) = 0;
    virtual bool isClicked(void) = 0;
    virtual void changeY(int, bool) = 0;
    virtual bool isFixed(void) = 0;
};

// A text displayable
class Text : public Displayable {
public:
    virtual void paint(XInfo& xinfo) {
        if (!hide){
            XDrawImageString( xinfo.display, xinfo.window, xinfo.gc,
                              this->x, this->y, this->s.c_str(), this->s.length() );
        }
    }
    virtual void mouseover(int, int){}
    virtual void mouseclick(int, int){}
    Text(int x, int y, string s, bool fixed): x(x), y(y), originalY(y), s(s), fixed(fixed)  {}
    string getType(){return "Text";}
    int getOrder(){return stoi(s);}
    string getStr(){return s;}
    int getX(){return x;}
    int getY(){return y;}
    void changebool(bool b){hide = b;}
    bool isClicked(){return false;}
    void changeY(int a, bool restore){if (!restore){y = a;}else{y = originalY;}}
    bool isFixed(){return fixed;}
private:
    int x;
    int y;
    int originalY;
    bool fixed;
    string s; // string to show

    bool hide = false;
};

// A button displayable
class Button : public Displayable{
public:
    // the VIEW
    virtual void paint(XInfo& xinfo){
        if (isOn) {
            setForeground(WHITE);
            XFillArc(xinfo.display, xinfo.window, xinfo.gc,
                     x - (d / 2), y - (d / 2), d, d, 0, 360 * 64);
            setForeground(BLACK);
            XDrawArc(xinfo.display, xinfo.window, gc[1],
                     x - (d / 2), y - (d / 2), d, d, 0, 360 * 64);
        }
        if (Clicked){
            setForeground(BLACK);
            XFillArc(xinfo.display, xinfo.window, xinfo.gc,
                     x - (d / 2), y - (d / 2), d, d, 0, 360 * 64);
            setForeground(BLACK);
            XDrawArc(xinfo.display, xinfo.window, xinfo.gc,
                     x - (d / 2), y - (d / 2), d, d, 0, 360 * 64);


        }
        if(!isOn && !Clicked){
            setForeground(WHITE);
            XFillArc(xinfo.display, xinfo.window, xinfo.gc,
                     x - (d / 2), y - (d / 2), d, d, 0, 360 * 64);
            setForeground(BLACK);
            XDrawArc(xinfo.display, xinfo.window, xinfo.gc,
                     x - (d / 2), y - (d / 2), d, d, 0, 360 * 64);
        }

    }

    // the CONTROLLER
    void mouseover(int mx, int my) {
        float dist = sqrt(pow(mx - x, 2) + pow(my - y, 2));
        if (dist < d/2) {
            isOn = true;
        } else {
            isOn = false;
            }
    }

    void mouseclick(int mx, int my){
        float dist = sqrt(pow(mx - x, 2) + pow(my - y, 2));
        if (dist < d/2) {
            toggle();
            clicked_counter = order;
            clicked_x = x;
            clicked_y = y;
            lock = true;
        } else {
            //Clicked = false;
        }
    }

    Button(int x, int y, int d, int order, void(*toggleEvent)(Button, bool)): x(x), y(y), d(d), originalY(y), order(order), toggleEvent(toggleEvent) {}

    int getX(){return x;}
    int getY(){return y;}
    int getD(){return d;}
    int getOrder(){return order;}
    string getType(){return "Button";}
    string getStr(){return itoa(order, 10);}
    void changebool(bool b){Clicked = b;}
    bool isClicked(){return Clicked;}
    void changeY(int a, bool restore){if (!restore){y = a;}else{y = originalY;}}
    bool isFixed(){return true;}
private:
    int order;
    int x;
    int y;
    int d;
    const int originalY;
    // toggle event callback
    void (*toggleEvent)(Button, bool);

    // the MODEL
    bool isOn = false;
    bool Clicked = false;

    void toggle() {
        Clicked = !Clicked;
        toggleEvent(*this, Clicked);
    }

};

list<Displayable*> dList;



void repaint( list<Displayable*> dList) {
    list<Displayable*>::const_iterator begin = dList.begin();
    list<Displayable*>::const_iterator end = dList.end();
    XClearWindow(xinfo.display, xinfo.window);
    dList.push_back(new Text(50,50,itoa(score,10),true));
    dList.push_back(new Text(50,100,message,true));
    while( begin != end ) {
        setForeground(BLACK);
        Displayable* d = *begin;
        d->changeY(0, true);
        d->paint(xinfo);
        begin++;
        if (frame_counter == 1){

            if (d->getType() == "Text"){
                d->changebool(false);
            }
            if ((d->getType() == "Button")&&(d->isClicked())){
                d->changebool(false);
            }
            lock = false;

        }
    }
    if ((clicked_counter > 0)&&(frame_counter >= 0)){
        int d = 100 / 30 * frame_counter;
        setForeground(WHITE);
        XDrawArc(xinfo.display, xinfo.window, xinfo.gc,
                 clicked_x - d/2, clicked_y- d/2,  d, d, 0,  360 * 64);
        setForeground(BLACK);
            frame_counter--;
    }
    XFlush(xinfo.display);
}

void sinRepaint( list<Displayable*> dList, int buttons) {
    list<Displayable*>::const_iterator begin = dList.begin();
    list<Displayable*>::const_iterator end = dList.end();
    XClearWindow(xinfo.display, xinfo.window);
    dList.push_back(new Text(50,50,itoa(score,10),true));
    dList.push_back(new Text(50,100,message,true));
    XWindowAttributes winattr;
    XGetWindowAttributes(xinfo.display, xinfo.window, &winattr);
    int w = winattr.width;
    int h = winattr.height;
    float newY;
    while( begin != end ) {
        setForeground(BLACK);
        Displayable* d = *begin;
        if ((d->getType() == "Text")&&(!d->isFixed())){
           newY = 10+h/2+15*sin(2*M_PI/buttons*(d->getOrder())+0.000005*now());
        } else if (d->getType() == "Button"){
            newY = h/2+15*sin(2*M_PI/buttons*(d->getOrder())+0.000005*now());
        } else {
            newY = d->getY();
        }
        d->changeY(newY, false);
        d->paint(xinfo);
        begin++;
        if (frame_counter == 1){

            if (d->getType() == "Text"){
                d->changebool(false);
            }
            if ((d->getType() == "Button")&&(d->isClicked())){
                d->changebool(false);
            }
            lock = false;

        }
    }
    if ((clicked_counter > 0)&&(frame_counter >= 0)){
        int d = 100 / 30 * frame_counter;
        setForeground(WHITE);
        XDrawArc(xinfo.display, xinfo.window, xinfo.gc,
                 clicked_x - d/2, clicked_y- d/2,  d, d, 0,  360 * 64);
        setForeground(BLACK);
        frame_counter--;

    }
    XFlush(xinfo.display);
}

/* Initialize the game window*/
void initWindow(int argc, char* argv[], int n) {
    /*
    * Display opening uses the DISPLAY	environment variable.
    * It can go wrong if DISPLAY isn't set, or you don't have permission.
    */
    xinfo.display = XOpenDisplay( "" );
    if ( !xinfo.display )	{
        error( "Can't open display." );
        exit(-1);
    }

    /*
    * Find out some things about the display you're using.
    */
    // DefaultScreen is as macro to get default screen index
    xinfo.screen = DefaultScreen( xinfo.display );

    int w = 800;
    int h = 400;
    unsigned long white = XWhitePixel( xinfo.display, xinfo.screen );
    unsigned long black = XBlackPixel( xinfo.display, xinfo.screen );

    xinfo.window = XCreateSimpleWindow(
            xinfo.display,				// display where window appears
            DefaultRootWindow( xinfo.display ), // window's parent in window tree
            10, 10,			            // upper left corner location
            w, h,	                // size of the window
            5,						    // width of window's border
            black,						// window border colour
            white );					    // window background colour

    // extra window properties like a window title
    XSetStandardProperties(
            xinfo.display,		// display containing the window
            xinfo.window,		// window whose properties are set
            "a1",	// window's title
            "OW",				// icon's title
            None,				// pixmap for the icon
            argv, argc,			// applications command line args
            None );			    // size hints for the window

    // drawing demo with graphics context here ...



// Create 3 Graphics Contexts
    int i = 0;
    gc[i] = XCreateGC(xinfo.display, xinfo.window, 0, 0);
    XSetForeground(xinfo.display, gc[i], BlackPixel(xinfo.display, xinfo.screen));
    XSetBackground(xinfo.display, gc[i], WhitePixel(xinfo.display, xinfo.screen));
    XSetFillStyle(xinfo.display, gc[i], FillSolid);
    XSetLineAttributes(xinfo.display, gc[i],
                       1, LineSolid, CapButt, JoinRound);

    i = 1;
    gc[i] = XCreateGC(xinfo.display, xinfo.window, 0, 0);
    XSetForeground(xinfo.display, gc[i], BlackPixel(xinfo.display, xinfo.screen));
    XSetBackground(xinfo.display, gc[i], WhitePixel(xinfo.display, xinfo.screen));
    XSetFillStyle(xinfo.display, gc[i], FillSolid);
    XSetLineAttributes(xinfo.display, gc[i],
                       4, LineSolid, CapRound, JoinMiter);

    xinfo.gc = gc[0];       // create a graphics context
    XSetForeground(xinfo.display, xinfo.gc, BlackPixel(xinfo.display, xinfo.screen));
    XSetBackground(xinfo.display, xinfo.gc, WhitePixel(xinfo.display, xinfo.screen));

    //load a larger font

    XFontStruct * font;
    font = XLoadQueryFont (xinfo.display, "12x24");
    XSetFont (xinfo.display, xinfo.gc, font->fid);
    XSelectInput(xinfo.display, xinfo.window,
                 ButtonPressMask | KeyPressMask | StructureNotifyMask | ButtonMotionMask | PointerMotionMask); // select events

    /* * Put the window on the screen. */
    XMapRaised( xinfo.display, xinfo.window );
    XFlush(xinfo.display);
    sleep(1); //sleep for 10ms


}


void ClickButton(Button b, bool clicked){
    int o = b.getOrder();
    cout << endl;
    list<Displayable*>::const_iterator begin = dList.begin();
    list<Displayable*>::const_iterator end = dList.end();
    while (begin != end){
        Displayable* d = *begin;
        if ((d->getType() == "Text")&&(stoi(d->getStr()) == o)&&(clicked)) {
            d->changebool(true);
        } else if ((d->getType() == "Text")&&(stoi(d->getStr()) != o)){
            d->changebool(false);
        }
        begin++;
    }

}

void Press(int x, int y, Simon::State s, int n){
    clicked_counter = 0;
    frame_counter = 30;
    list<Displayable *>::const_iterator begin = dList.begin();
    list<Displayable *>::const_iterator end = dList.end();
    while (begin != end) {
            Displayable *d = *begin;
            if (s == Simon::HUMAN) {
                if (d->getType() == "Button") {
                    d->mouseclick(x, y);
                }
            } else if (s == Simon::COMPUTER) {
                if ((d->getType() == "Button") && (d->getOrder() == n)) {
                    d->mouseclick(d->getX(), d->getY());
                }

        }

        begin++;
    }
    begin = dList.begin();
    if (clicked_counter == 0) {
        while (begin != end) {
            Displayable *d = *begin;
            if (d->getType() == "Text") {
                d->changebool(false);
            }
            begin++;
        }
    }
}


void eventloop(int n){
    // time of last xinfo.window paint
    unsigned long lastRepaint = 0;
    Simon simon = Simon(n, true);
    XEvent event;
    int nb = 0;
    int lastpress = 0;
    while (true) {
            delay++;
        if (delay > 100000000){
            delay = 0;
            lastpress = 0;
        }

        if (XPending(xinfo.display) > 0) {
            XNextEvent( xinfo.display, &event );
            switch ( event.type ) {
                case KeyPress:{ // any keypress
                    KeySym key;
                    char text[10];
                    int i = XLookupString( (XKeyEvent*)&event, text, 10, &key, 0 );
                    if ((key == XK_q)||(key == XK_Q)) {
                        XCloseDisplay(xinfo.display);
                        cout << "EXIT" << endl;
                        exit(0);
                    }
                    if (key == XK_space && (simon.getState()==Simon::START || simon.getState()==Simon::WIN || simon.getState()==Simon::LOSE)){
                        simon.newRound();
                    }
                }
                    break;

                case ConfigureNotify:{ //Resize Window
                    XWindowAttributes winattr;
                    XGetWindowAttributes(xinfo.display, xinfo.window, &winattr);
                    dList.clear();

                    int w = winattr.width;
                    int h = winattr.height;
                    int interval = (w - n * 100)/(n+1);

                    dList.push_back(new Button(interval + 50, h/2, 100, 1, &ClickButton));

                    dList.push_back(new Text(interval + 45,h/2+10,"1", false));
                    for (int i=2; i<=n; i++){
                        dList.push_back(new Button(i * interval + (i-1) * 100 + 50, h/2, 100, i, &ClickButton));
                        dList.push_back(new Text(i * interval + (i-1) * 100 + 45,h/2+10,itoa(i,10), false));
                    }
                    repaint(dList);}
                    break;

                case MotionNotify:{
                    if (simon.getState()!=Simon::COMPUTER) {
                        list<Displayable *>::const_iterator begin = dList.begin();
                        list<Displayable *>::const_iterator end = dList.end();
                        while (begin != end) {
                            Displayable *d = *begin;
                            if (d->getType() == "Button")
                                d->mouseover(event.xbutton.x, event.xbutton.y);
                            begin++;
                        }
                    }
                } break;

                case ButtonPress:{
                    if (!lock) {

                        if (simon.getState() == Simon::HUMAN) {
                            Press(event.xbutton.x, event.xbutton.y, Simon::HUMAN, 0);
                            if (clicked_counter != 0) {
                                simon.verifyButton(clicked_counter);
                            }
                        }
                    }


                } break;
            }
        }
        unsigned long end = now();

        if (end - lastRepaint > 1000000 / fps) {
            if ((simon.getState()==Simon::COMPUTER) || (simon.getState()==Simon::HUMAN)){
                repaint(dList);
            } else {
            sinRepaint(dList, n);
            }
            lastRepaint = now();
        }
        // IMPORTANT: sleep for a bit to let other processes work
        if (XPending(xinfo.display) == 0) {
            usleep(1000000 / fps - (end - lastRepaint));
        }


        // just output the values
        // (obviously this demo is not very challenging)
        if (simon.getState() == Simon::COMPUTER) {
            if (delay - lastpress > 45){
                Press(0, 0, Simon::COMPUTER, simon.nextButton());
                XSelectInput(xinfo.display, xinfo.window,
                             ButtonPressMask | KeyPressMask | StructureNotifyMask | ButtonMotionMask);
                lastpress = delay;
            }
            message = "Watch what I do...";
        }

        // now human plays
        if (simon.getState() == Simon::HUMAN) {
            if (delay - lastpress > 30){
                XSelectInput(xinfo.display, xinfo.window,
                             ButtonPressMask | KeyPressMask | StructureNotifyMask | ButtonMotionMask | PointerMotionMask);
                message = "Your turn ...";
            }


        }
        if (simon.getState() == Simon::LOSE){
            message = "You lose. Press SPACE to play again";
        }

        if (simon.getState() == Simon::WIN){
            message = "You won. Press SPACE to continue";
            score = simon.getScore();
        }
    }
}

int main ( int argc, char* argv[] ) {
	// get the number of buttons from args
	// (default to 4 if no args)
	int n = 4;
    if (argc > 1) {
        n = atoi(argv[1]);
    }
    n = max(1, min(n, 6));


    initWindow(argc, argv, n);
    eventloop(n);
    // wait for user input to quit (a concole event for now)

}