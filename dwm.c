/* See LICENSE file for copyright and license details.
 *
 * dynamic window manager is designed like any other X client as well. It is
 * driven through handling X events. In contrast to other X clients, a window
 * manager selects for SubstructureRedirectMask on the root window, to receive
 * events about window (dis-)appearance. Only one X connection at a time is
 * allowed to select for this event mask.
 *
 * The event handlers of dwm are organized in an array which is accessed
 * whenever a new event has been fetched. This allows event dispatching
 * in O(1) time.
 *
 * Each child of the root window is called a client, except windows which have
 * set the override_redirect flag. Clients are organized in a linked client
 * list on each monitor, the focus history is remembered through a stack list
 * on each monitor. Each client contains a bit array to indicate the tags of a
 * client.
 *
 * Keys and tagging rules are organized as arrays and defined in config.h.
 *
 * To understand everything else, start reading main().
 */
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <errno.h>
#include <locale.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif /* XINERAMA */
#include <X11/Xft/Xft.h>

#include "drw.h"
#include "util.h"

/* macros */
#define BUTTONMASK               (ButtonPressMask | ButtonReleaseMask)
#define CLEANMASK(mask)          (mask & ~(numlockmask | LockMask) & (ShiftMask | ControlMask | Mod1Mask | Mod2Mask | Mod3Mask | Mod4Mask | Mod5Mask))
#define INTERSECT(x, y, w, h, m) (MAX(0, MIN((x) + (w), (m)->wx + (m)->ww) - MAX((x), (m)->wx)) \
                                  * MAX(0, MIN((y) + (h), (m)->wy + (m)->wh) - MAX((y), (m)->wy)))
#define ISVISIBLEONTAG(C, T) ((C->tags & T))
#define ISVISIBLE(C)         ((C->tags & C->mon->tagset[C->mon->seltags]))
#define HIDDEN(C)            ((getstate(C->win) == IconicState))
#define LENGTH(X)            (sizeof X / sizeof X[0])
#define MOUSEMASK            (BUTTONMASK | PointerMotionMask)
#define WIDTH(X)             ((X)->w + 2 * (X)->bw)
#define HEIGHT(X)            ((X)->h + 2 * (X)->bw)
#define TAGMASK              ((1 << LENGTH(tags)) - 1)
#define TEXTW(X, F)          (drw_fontset_getwidth(drw, (X), (F)) + lrpad)
#define OPAQUE               0xffU

/* enums */
enum {
    CurNormal,
    CurResize,
    CurMove,
    CurHand,
    CurLast
}; /* cursor */
enum {
    SchemeNorm,
    SchemeSel,
    SchemeStatus,
    SchemeTagsSel,
    SchemeTagsHover,
    SchemeTagsNorm,
    SchemeInfoSel,
    SchemeInfoNorm
}; /* color schemes */
enum {
    NetSupported,
    NetWMName,
    NetWMState,
    NetWMCheck,
    NetWMFullscreen,
    NetActiveWindow,
    NetWMWindowType,
    NetWMWindowTypeDialog,
    NetClientList,
    NetLast
}; /* EWMH atoms */
enum {
    WMProtocols,
    WMDelete,
    WMState,
    WMTakeFocus,
    WMLast
}; /* default atoms */
enum {
    ClkTagBar,
    ClkLtSymbol,
    ClkStatusText,
    ClkWinTitle,
    ClkClientWin,
    ClkRootWin,
    ClkLast
}; /* clicks */
enum {
    WIN_N,
    WIN_W,
    WIN_C,
    WIN_E,
    WIN_S,
}; /* coordinates for movethrow */
enum {
    OPEN_CLIENT,
    CLOSE_CLIENT,
}; /* actions of unfloatexceptlatest */

typedef union {
    int i;
    unsigned int ui;
    float f;
    const void *v;
} Arg;

typedef struct
{
    unsigned int click;
    unsigned int mask;
    unsigned int button;
    void (*func)(const Arg *arg);
    const Arg arg;
} Button;

typedef struct Monitor Monitor;
typedef struct Client Client;
struct Client {
    char name[256];
    float mina, maxa;
    int x, y, w, h;
    int oldx, oldy, oldw, oldh;
    int basew, baseh, incw, inch, maxw, maxh, minw, minh, hintsvalid;
    int bw, oldbw;
    unsigned int tags;
    int isfixed, isfloating, isbehide, isurgent, neverfocus, oldstate, isfullscreen, ispreventtile, iswarppointer;
    int borderpx;
    int hasrulebw;
    Client *next;
    Client *snext;
    Monitor *mon;
    Window win;
};

typedef struct
{
    unsigned int mod;
    KeySym keysym;
    void (*func)(const Arg *);
    const Arg arg;
} Key;

typedef struct
{
    const char *symbol;
    void (*arrange)(Monitor *);
} Layout;

typedef struct Pertag Pertag;
struct Monitor {
    char ltsymbol[16];
    float mfact;
    int nmaster;
    int num;
    int by;             /* bar geometry */
    int mx, my, mw, mh; /* screen size */
    int wx, wy, ww, wh; /* window area  */
    int gappx;          /* gaps between windows */
    unsigned int seltags;
    unsigned int sellt;
    unsigned int tagset[2];
    int rmaster;
    int showbar;
    int topbar;
    int hidsel;
    Client *clients;
    Client *sel;
    Client *stack;
    Monitor *next;
    Window barwin;
    const Layout *lt[3];
    Pertag *pertag;
};

typedef struct
{
    const char *class;
    const char *instance;
    const char *title;
    unsigned int tags;
    int isfloating;
    int ispreventtile;
    int monitor;
    int isfactor;
    double factorx, factory, factorw, factorh;
    int borderpx;
    int iswarppointer;
} Rule;

/* function declarations */
static void applyfactor(Client *c, const Rule *r);
static void applyrules(Client *c);
static int applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact);
static void arrange(Monitor *m);
static void arrangemon(Monitor *m);
static void attach(Client *c);
static void attachabove(Client *c);
static void attachaside(Client *c);
static void attachbelow(Client *c);
static void attachbottom(Client *c);
static void attachtop(Client *c);
static void attachstack(Client *c);
static void buttonpress(XEvent *e);
static void changerule(Client *c);
static void checkotherwm(void);
static void cleanup(void);
static void cleanupmon(Monitor *mon);
static void clientmessage(XEvent *e);
static void configure(Client *c);
static void configurenotify(XEvent *e);
static void configurerequest(XEvent *e);
static Monitor *createmon(void);
static void cyclelayout(const Arg *arg);
static void destroynotify(XEvent *e);
static void detach(Client *c);
static void detachstack(Client *c);
static Monitor *dirtomon(int dir);
static void dotogglefloating(Monitor *m, Client *c);
static void drawbar(Monitor *m);
static void drawbars(void);
static void drawhoverbar(Monitor *m, XMotionEvent *ev);
static void drawhoverbars(XMotionEvent *ev);
static void enqueue(Client *c);
static void enqueuestack(Client *c);
static void enternotify(XEvent *e);
static void expose(XEvent *e);
static void focus(Client *c);
static void focusin(XEvent *e);
static void focusmon(const Arg *arg);
static void focusstackvis(const Arg *arg);
static void focusstackhid(const Arg *arg);
static void focusstack(int inc, int vis);
static Atom getatomprop(Client *c, Atom prop);
static int getrootptr(int *x, int *y);
static long getstate(Window w);
static int gettextprop(Window w, Atom atom, char *text, unsigned int size);
static void grabbuttons(Client *c, int focused);
static void grabkeys(void);
static void hide(const Arg *arg);
static void hideall(const Arg *arg);
static void hidewin(Client *c);
static int ispanel(Client *c);
static int isnotifyd(Client *c);
static void incnmaster(const Arg *arg);
static void keypress(XEvent *e);
static void killclient(const Arg *arg);
static void manage(Window w, XWindowAttributes *wa);
static void mappingnotify(XEvent *e);
static void maprequest(XEvent *e);
static void monocle(Monitor *m);
static void motionnotify(XEvent *e);
static void movemouse(const Arg *arg);
static void movethrow(const Arg *arg);
static Client *nexttagged(Client *c);
static Client *nexttiled(Client *c);
static void pop(Client *c);
static void propertynotify(XEvent *e);
static void quit(const Arg *arg);
static Monitor *recttomon(int x, int y, int w, int h);
static void resetnmaster(const Arg *arg);
static void resize(Client *c, int x, int y, int w, int h, int interact);
static void resizebarwin(Monitor *m);
static void resizeclient(Client *c, int x, int y, int w, int h);
static void resizemouse(const Arg *arg);
static void restack(Monitor *m);
static void rotatestack(const Arg *arg);
static void run(void);
static void runautostart(void);
static void scan(void);
static int sendevent(Client *c, Atom proto);
static void sendmon(Client *c, Monitor *m);
static void setclientstate(Client *c, long state);
static void setfocus(Client *c);
static void setfullscreen(Client *c, int fullscreen);
static void setgaps(const Arg *arg);
static void setlayout(const Arg *arg);
static void setmfact(const Arg *arg);
static void setup(void);
static void seturgent(Client *c, int urg);
static void show(const Arg *arg);
static void showall(const Arg *arg);
static void showhide(Client *c);
static void showwin(Client *c);
static void sigchld(int unused);
static void spawn(const Arg *arg);
static void tag(const Arg *arg);
static void tagmon(const Arg *arg);
static void tile(Monitor *m);
static void togglebar(const Arg *arg);
static void togglefloating(const Arg *arg);
static void togglebehide(const Arg *arg);
static void togglermaster(const Arg *arg);
static void toggletag(const Arg *arg);
static void toggleview(const Arg *arg);
static void unfloatexceptlatest(Monitor *m, Client *c, int action);
static void unfocus(Client *c, int setfocus);
static void unmanage(Client *c, int destroyed);
static void unmapnotify(XEvent *e);
static void updatebarpos(Monitor *m);
static void updatebars(void);
static void updateclientlist(void);
static int updategeom(void);
static void updatenumlockmask(void);
static void updatesizehints(Client *c);
static void updatestatus(void);
static void updatetitle(Client *c);
static void updatewindowtype(Client *c);
static void updatewmhints(Client *c);
static void view(const Arg *arg);
static void viewall(const Arg *arg);
static Client *wintoclient(Window w);
static Monitor *wintomon(Window w);
static void warppointer(Monitor *m);
static int xerror(Display *dpy, XErrorEvent *ee);
static int xerrordummy(Display *dpy, XErrorEvent *ee);
static int xerrorstart(Display *dpy, XErrorEvent *ee);
static void xinitvisual();
static void zoom(const Arg *arg);

/* variables */
static const char autostartblocksh[] = "autostart_blocking.sh";
static const char autostartsh[] = "autostart.sh";
static const char broken[] = "broken";
static const char dwmdir[] = "dwm";
static const char localshare[] = ".local/share";
static char stext[256];
static int screen;
static int sw, sh; /* X display screen geometry width, height */
static int bh;     /* bar height */
static int lrpad;  /* sum of left and right padding for text */
static int vp;     /* vertical padding for bar */
static int sp;     /* side padding for bar */
static int (*xerrorxlib)(Display *, XErrorEvent *);
static unsigned int numlockmask = 0;
static void (*handler[LASTEvent])(XEvent *) = {
    [ButtonPress] = buttonpress,
    [ClientMessage] = clientmessage,
    [ConfigureRequest] = configurerequest,
    [ConfigureNotify] = configurenotify,
    [DestroyNotify] = destroynotify,
    [EnterNotify] = enternotify,
    [Expose] = expose,
    [FocusIn] = focusin,
    [KeyPress] = keypress,
    [MappingNotify] = mappingnotify,
    [MapRequest] = maprequest,
    [MotionNotify] = motionnotify,
    [PropertyNotify] = propertynotify,
    [UnmapNotify] = unmapnotify};
static Atom wmatom[WMLast], netatom[NetLast];
static int running = 1;
static Cur *cursor[CurLast];
static Clr **scheme;
static Display *dpy;
static Drw *drw;
static Monitor *mons, *selmon;
static Window root, wmcheckwin;

static int useargb = 0;
static Visual *visual;
static int depth;
static Colormap cmap;
static int isfloating_src = 0;

pthread_mutex_t rule_mutex = PTHREAD_MUTEX_INITIALIZER;

/* configuration, allows nested code to access above variables */
#include "config.h"
#include <time.h>

struct Pertag {
    unsigned int curtag, prevtag;              /* current and previous tag */
    int nmasters[LENGTH(tags) + 1];            /* number of windows in master area */
    float mfacts[LENGTH(tags) + 1];            /* mfacts per tag */
    unsigned int sellts[LENGTH(tags) + 1];     /* selected layouts */
    const Layout *ltidxs[LENGTH(tags) + 1][2]; /* matrix of tags and layouts indexes  */
};

/* compile-time check if all tags fit into an unsigned int bit array. */
struct NumTags {
    char limitexceeded[LENGTH(tags) > 31 ? -1 : 1];
};

/* function implementations */
void
applyfactor(Client *c, const Rule *r)
{
    int cx, cy, cw, ch, x1, y1, x2, y2;
    Monitor *m = c->mon;
    cx = m->wx + m->gappx;
    cy = m->wy + m->gappx;
    cw = m->ww - 2 * c->bw - (2 * m->gappx);
    ch = m->wh - 2 * c->bw - (2 * m->gappx);
    x1 = cx;
    y1 = cy;
    if (r->factorx == 1.0 && r->factory <= 1.0) {
        x2 = x1 + cw - m->gappx - c->bw;
        y2 = y1 + ch;
    } else if (r->factory == 1.0 && r->factorx <= 1.0) {
        x2 = x1 + cw;
        y2 = y1 + ch - bh - m->gappx - c->bw;
    } else {
        x2 = x1 + cw;
        y2 = y1 + ch;
    }
    resizeclient(c,
                 r->factorx == 1.0 ? x1 : (r->factorx == 0.0 ? x1 : x2 * (1 - r->factorx)),
                 r->factory == 1.0 ? y1 : (r->factory == 0.0 ? y1 : y2 * (1 - r->factory)),
                 ((x2 * r->factorx) == x2 ? cw : (r->factorx == 0.0 ? cw : x2 * r->factorx)) * (r->factorw == 0.0 ? 1.0 : r->factorw),
                 ((y2 * r->factory) == y2 ? ch : (r->factory == 0.0 ? ch : y2 * r->factory)) * (r->factorh == 0.0 ? 1.0 : r->factorh));
}

void
applyrules(Client *c)
{
    const char *class, *instance;
    unsigned int i;
    const Rule *r;
    Monitor *m;
    XClassHint ch = {NULL, NULL};

    /* rule matching */
    c->iswarppointer = 1;
    c->isfloating = 0;
    c->ispreventtile = 1;
    c->tags = 0;
    XGetClassHint(dpy, c->win, &ch);
    class = ch.res_class ? ch.res_class : broken;
    instance = ch.res_name ? ch.res_name : broken;
    for (i = 0; i < LENGTH(rules); i++) {
        r = &rules[i];
        if ((!r->title || strstr(c->name, r->title))
            && (!r->class || strstr(class, r->class))
            && (!r->instance || strstr(instance, r->instance))) {
            // the `!(c->mon->num)` is a primary or first monitor
            c->isfloating = !(c->mon->num) ? r->isfloating : c->isfloating;
            c->ispreventtile = r->ispreventtile;
            c->tags |= r->tags;
            c->iswarppointer = r->iswarppointer;
            isfloating_src = c->isfloating;

            if (c->isfloating && !ispanel(c)) {
                if (r->isfactor)
                    applyfactor(c, r);
            }

            if (r->borderpx >= 0) {
                c->borderpx = r->borderpx;
                c->hasrulebw = 1;
            }

            for (m = mons; m && m->num != r->monitor; m = m->next)
                ;
            if (m)
                c->mon = m;
        }
    }
    if (ch.res_class)
        XFree(ch.res_class);
    if (ch.res_name)
        XFree(ch.res_name);
    c->tags = c->tags & TAGMASK ? c->tags & TAGMASK : c->mon->tagset[c->mon->seltags];
}

int
applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact)
{
    int baseismin;
    Monitor *m = c->mon;

    /* set minimum possible */
    *w = MAX(1, *w);
    *h = MAX(1, *h);
    if (interact) {
        if (*x > sw)
            *x = sw - WIDTH(c);
        if (*y > sh)
            *y = sh - HEIGHT(c);
        if (*x + *w + 2 * c->bw < 0)
            *x = 0;
        if (*y + *h + 2 * c->bw < 0)
            *y = 0;
    } else {
        if (*x >= m->wx + m->ww)
            *x = m->wx + m->ww - WIDTH(c);
        if (*y >= m->wy + m->wh)
            *y = m->wy + m->wh - HEIGHT(c);
        if (*x + *w + 2 * c->bw <= m->wx)
            *x = m->wx;
        if (*y + *h + 2 * c->bw <= m->wy)
            *y = m->wy;
    }
    if (*h < bh)
        *h = bh;
    if (*w < bh)
        *w = bh;
    if (resizehints || c->isfloating || !c->mon->lt[c->mon->sellt]->arrange) {
        if (!c->hintsvalid)
            updatesizehints(c);
        /* see last two sentences in ICCCM 4.1.2.3 */
        baseismin = c->basew == c->minw && c->baseh == c->minh;
        if (!baseismin) { /* temporarily remove base dimensions */
            *w -= c->basew;
            *h -= c->baseh;
        }
        /* adjust for aspect limits */
        if (c->mina > 0 && c->maxa > 0) {
            if (c->maxa < (float)*w / *h)
                *w = *h * c->maxa + 0.5;
            else if (c->mina < (float)*h / *w)
                *h = *w * c->mina + 0.5;
        }
        if (baseismin) { /* increment calculation requires this */
            *w -= c->basew;
            *h -= c->baseh;
        }
        /* adjust for increment value */
        if (c->incw)
            *w -= *w % c->incw;
        if (c->inch)
            *h -= *h % c->inch;
        /* restore base dimensions */
        *w = MAX(*w + c->basew, c->minw);
        *h = MAX(*h + c->baseh, c->minh);
        if (c->maxw)
            *w = MIN(*w, c->maxw);
        if (c->maxh)
            *h = MIN(*h, c->maxh);
    }
    return *x != c->x || *y != c->y || *w != c->w || *h != c->h;
}

void
arrange(Monitor *m)
{
    XEvent ev;
    if (m)
        showhide(m->stack);
    else
        for (m = mons; m; m = m->next)
            showhide(m->stack);
    if (m) {
        arrangemon(m);
        restack(m);
    } else {
        for (m = mons; m; m = m->next)
            arrangemon(m);
        XSync(dpy, False);
        while (XCheckMaskEvent(dpy, EnterWindowMask, &ev))
            ;
    }
}

void
arrangemon(Monitor *m)
{
    strncpy(m->ltsymbol, m->lt[m->sellt]->symbol, sizeof m->ltsymbol);
    if (m->lt[m->sellt]->arrange)
        m->lt[m->sellt]->arrange(m);
}

void
attach(Client *c)
{
    c->next = c->mon->clients;
    c->mon->clients = c;
}

void
attachabove(Client *c)
{
    if (c->mon->sel == NULL || c->mon->sel == c->mon->clients || c->mon->sel->isfloating) {
        attach(c);
        return;
    }

    Client *at;
    for (at = c->mon->clients; at->next != c->mon->sel; at = at->next)
        ;
    c->next = at->next;
    at->next = c;
}

void
attachaside(Client *c)
{
    Client *at = nexttagged(c);
    if (!at) {
        attach(c);
        return;
    }
    c->next = at->next;
    at->next = c;
}

void
attachbelow(Client *c)
{
    if (c->mon->sel == NULL || c->mon->sel == c || c->mon->sel->isfloating) {
        attach(c);
        return;
    }
    c->next = c->mon->sel->next;
    c->mon->sel->next = c;
}

void
attachbottom(Client *c)
{
    Client *below = c->mon->clients;
    for (; below && below->next; below = below->next)
        ;
    c->next = NULL;
    if (below)
        below->next = c;
    else
        c->mon->clients = c;
}

void
attachtop(Client *c)
{
    int n;
    Monitor *m = selmon;
    Client *below;

    for (n = 1, below = c->mon->clients;
         below && below->next && (below->isfloating || !ISVISIBLEONTAG(below, c->tags) || n != m->nmaster);
         n = below->isfloating || !ISVISIBLEONTAG(below, c->tags) ? n + 0 : n + 1, below = below->next)
        ;
    c->next = NULL;
    if (below) {
        c->next = below->next;
        below->next = c;
    } else
        c->mon->clients = c;
}

void
attachstack(Client *c)
{
    c->snext = c->mon->stack;
    c->mon->stack = c;
}

void
buttonpress(XEvent *e)
{
    unsigned int i, x, click, occ;
    Arg arg = {0};
    Client *c;
    Monitor *m;
    XButtonPressedEvent *ev = &e->xbutton;

    click = ClkRootWin;
    /* focus monitor if necessary */
    if ((m = wintomon(ev->window)) && m != selmon) {
        unfocus(selmon->sel, 1);
        selmon = m;
        focus(NULL);
    }
    if (ev->window == selmon->barwin) {
        i = x = occ = 0;
        for (c = m->clients; c; c = c->next) {
            if (ispanel(c)) continue;
            occ |= c->tags;
        }
        do {
            /* Do not reserve space for vacant tags */
            if (i > min_tag - 1 && !(occ & 1 << i || m->tagset[m->seltags] & 1 << i))
                continue;
            x += TEXTW(tags[i], 0);
        } while (ev->x >= x && ++i < LENGTH(tags));
        if (i < LENGTH(tags)) {
            click = ClkTagBar;
            arg.ui = 1 << i;
        } else if (ev->x < x + TEXTW(selmon->ltsymbol, 0))
            click = ClkLtSymbol;
        else if (ev->x > selmon->ww - (int)TEXTW(stext, 0))
            click = ClkStatusText;
        else
            click = ClkWinTitle;
    } else if ((c = wintoclient(ev->window))) {
        focus(c);
        restack(selmon);
        XAllowEvents(dpy, ReplayPointer, CurrentTime);
        click = ClkClientWin;
    }
    for (i = 0; i < LENGTH(buttons); i++)
        if (click == buttons[i].click && buttons[i].func && buttons[i].button == ev->button
            && CLEANMASK(buttons[i].mask) == CLEANMASK(ev->state))
            buttons[i].func(click == ClkTagBar && buttons[i].arg.i == 0 ? &arg : &buttons[i].arg);
}

void
changerule(Client *c)
{
    if (!c)
        return;
    Monitor *m = c->mon;
    const char *class, *instance;
    unsigned int i;
    Rule *r;
    XClassHint ch = {NULL, NULL};
    XGetClassHint(dpy, c->win, &ch);
    class = ch.res_class ? ch.res_class : broken;
    instance = ch.res_name ? ch.res_name : broken;
    pthread_mutex_lock(&rule_mutex);
    for (i = 0; i < LENGTH(rules); i++) {
        r = &rules[i];
        if ((!r->title || strstr(c->name, r->title))
            && (!r->class || strstr(class, r->class))
            && (!r->instance || strstr(instance, r->instance))) {
            if (dynamicrule) {
                if (!r->ispreventtile)
                    r->isfloating ^= 1;
            }

            if (!ispanel(c)) {
                // the `!m->num` is a primary or first monitor
                if (c->isfloating && !m->num)
                    applyfactor(c, r);
            }
        }
    }
    pthread_mutex_unlock(&rule_mutex);
    if (ch.res_class)
        XFree(ch.res_class);
    if (ch.res_name)
        XFree(ch.res_name);
}

void
checkotherwm(void)
{
    xerrorxlib = XSetErrorHandler(xerrorstart);
    /* this causes an error if some other window manager is running */
    XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureRedirectMask);
    XSync(dpy, False);
    XSetErrorHandler(xerror);
    XSync(dpy, False);
}

void
cleanup(void)
{
    Arg a = {.ui = ~0};
    Layout foo = {"", NULL};
    Monitor *m;
    size_t i;

    view(&a);
    selmon->lt[selmon->sellt] = &foo;
    for (m = mons; m; m = m->next)
        while (m->stack)
            unmanage(m->stack, 0);
    XUngrabKey(dpy, AnyKey, AnyModifier, root);
    while (mons)
        cleanupmon(mons);

    for (i = 0; i < CurLast; i++)
        drw_cur_free(drw, cursor[i]);
    for (i = 0; i < LENGTH(colors); i++)
        free(scheme[i]);
    free(scheme);
    XDestroyWindow(dpy, wmcheckwin);
    drw_free(drw);
    XSync(dpy, False);
    XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
    XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
}

void
cleanupmon(Monitor *mon)
{
    Monitor *m;

    if (mon == mons)
        mons = mons->next;
    else {
        for (m = mons; m && m->next != mon; m = m->next)
            ;
        m->next = mon->next;
    }
    XUnmapWindow(dpy, mon->barwin);
    XDestroyWindow(dpy, mon->barwin);
    free(mon);
}

void
clientmessage(XEvent *e)
{
    XClientMessageEvent *cme = &e->xclient;
    Client *c = wintoclient(cme->window);

    if (!c)
        return;
    if (cme->message_type == netatom[NetWMState]) {
        if (cme->data.l[1] == netatom[NetWMFullscreen]
            || cme->data.l[2] == netatom[NetWMFullscreen])
            setfullscreen(c, (cme->data.l[0] == 1 /* _NET_WM_STATE_ADD    */
                              || cme->data.l[0] == 2 /* _NET_WM_STATE_TOGGLE */));
    } else if (cme->message_type == netatom[NetActiveWindow]) {
        if (c != selmon->sel && !c->isurgent)
            seturgent(c, 1);
    }
}

void
configure(Client *c)
{
    XConfigureEvent ce;

    ce.type = ConfigureNotify;
    ce.display = dpy;
    ce.event = c->win;
    ce.window = c->win;
    ce.x = c->x;
    ce.y = c->y;
    ce.width = c->w;
    ce.height = c->h;
    ce.border_width = c->bw;
    ce.above = None;
    ce.override_redirect = False;
    XSendEvent(dpy, c->win, False, StructureNotifyMask, (XEvent *)&ce);
}

void
configurenotify(XEvent *e)
{
    Monitor *m;
    XConfigureEvent *ev = &e->xconfigure;
    int dirty;

    /* TODO: updategeom handling sucks, needs to be simplified */
    if (ev->window == root) {
        dirty = (sw != ev->width || sh != ev->height);
        sw = ev->width;
        sh = ev->height;
        if (updategeom() || dirty) {
            drw_resize(drw, sw, bh);
            updatebars();
            for (m = mons; m; m = m->next)
                resizebarwin(m);
            focus(NULL);
            arrange(NULL);
        }
    }
}

void
configurerequest(XEvent *e)
{
    Client *c;
    Monitor *m;
    XConfigureRequestEvent *ev = &e->xconfigurerequest;
    XWindowChanges wc;

    if ((c = wintoclient(ev->window))) {
        if (ev->value_mask & CWBorderWidth)
            c->bw = ev->border_width;
        else if (c->isfloating || !selmon->lt[selmon->sellt]->arrange) {
            m = c->mon;
            if (ev->value_mask & CWX) {
                c->oldx = c->x;
                c->x = m->mx + ev->x;
            }
            if (ev->value_mask & CWY) {
                c->oldy = c->y;
                c->y = m->my + ev->y;
            }
            if (ev->value_mask & CWWidth) {
                c->oldw = c->w;
                c->w = ev->width;
            }
            if (ev->value_mask & CWHeight) {
                c->oldh = c->h;
                c->h = ev->height;
            }
            if ((c->x + c->w) > m->mx + m->mw && c->isfloating)
                c->x = m->mx + (m->mw / 2 - WIDTH(c) / 2);  /* center in x direction */
            if ((c->y + c->h) > m->my + m->mh && c->isfloating)
                c->y = m->my + (m->mh / 2 - HEIGHT(c) / 2); /* center in y direction */
            if ((ev->value_mask & (CWX | CWY)) && !(ev->value_mask & (CWWidth | CWHeight)))
                configure(c);
            if (ISVISIBLE(c))
                XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
        } else
            configure(c);
    } else {
        wc.x = ev->x;
        wc.y = ev->y;
        wc.width = ev->width;
        wc.height = ev->height;
        wc.border_width = ev->border_width;
        wc.sibling = ev->above;
        wc.stack_mode = ev->detail;
        XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
    }
    XSync(dpy, False);
}

Monitor *
createmon(void)
{
    Monitor *m;
    int i;

    m = ecalloc(1, sizeof(Monitor));
    m->tagset[0] = m->tagset[1] = 1;
    m->mfact = mfact;
    m->nmaster = nmaster;
    m->rmaster = rmaster;
    m->showbar = showbar;
    m->topbar = topbar;
    m->gappx = gappx;
    m->lt[0] = &layouts[0];
    m->lt[1] = &layouts[1 % LENGTH(layouts)];
    strncpy(m->ltsymbol, layouts[0].symbol, sizeof m->ltsymbol);
    if (!(m->pertag = (Pertag *)calloc(1, sizeof(Pertag))))
        die("fatal: could not malloc() %u bytes\n", sizeof(Pertag));
    m->pertag->curtag = m->pertag->prevtag = 1;
    for (i = 0; i <= LENGTH(tags); i++) {
        /* init nmaster */
        m->pertag->nmasters[i] = m->nmaster;

        /* init mfacts */
        m->pertag->mfacts[i] = m->mfact;

        /* init layouts */
        m->pertag->ltidxs[i][0] = m->lt[0];
        m->pertag->ltidxs[i][1] = m->lt[1];
        m->pertag->sellts[i] = m->sellt;
    }
    return m;
}

void
cyclelayout(const Arg *arg)
{
    Layout *l;
    for (l = (Layout *)layouts; l != selmon->lt[selmon->sellt]; l++)
        ;
    if (arg->i > 0)
        if (l->symbol && (l + 1)->symbol)
            setlayout(&((Arg){.v = (l + 1)}));
        else
            setlayout(&((Arg){.v = layouts}));
    else if (l != layouts && (l - 1)->symbol)
        setlayout(&((Arg){.v = (l - 1)}));
    else
        setlayout(&((Arg){.v = &layouts[LENGTH(layouts) - 2]}));
}

void
destroynotify(XEvent *e)
{
    Client *c;
    XDestroyWindowEvent *ev = &e->xdestroywindow;

    if ((c = wintoclient(ev->window)))
        unmanage(c, 1);
}

void
detach(Client *c)
{
    Client **tc;

    for (tc = &c->mon->clients; *tc && *tc != c; tc = &(*tc)->next)
        ;
    *tc = c->next;
}

void
detachstack(Client *c)
{
    Client **tc, *t;

    for (tc = &c->mon->stack; *tc && *tc != c; tc = &(*tc)->snext)
        ;
    *tc = c->snext;

    if (c == c->mon->sel) {
        for (t = c->mon->stack; t && !ISVISIBLE(t); t = t->snext)
            ;
        c->mon->sel = t;
    }
}

Monitor *
dirtomon(int dir)
{
    Monitor *m = NULL;

    if (dir > 0) {
        if (!(m = selmon->next))
            m = mons;
    } else if (selmon == mons)
        for (m = mons; m->next; m = m->next)
            ;
    else
        for (m = mons; m->next != selmon; m = m->next)
            ;
    return m;
}

void
dotogglefloating(Monitor *m, Client *c)
{
    if (!m || !c || ispanel(c))
        return;

    c->isfloating = !c->isfloating || c->mon->sel->isfixed;
    if (c->isfloating)
        resize(c, c->x, c->y, c->w, c->h, 0);

    changerule(c);
}

void
drawbar(Monitor *m)
{
    int x, w, tw = 0;
    int boxs = drw->fonts->h / 9;
    int boxw = drw->fonts->h / 6 + 2;
    unsigned int i, occ = 0, urg = 0;
    Client *c;

    if (!m->showbar)
        return;

    //	/* draw status first so it can be overdrawn by tags later */
    if (m == selmon) {                    /* status is only drawn on selected monitor */
        drw_setscheme(drw, scheme[SchemeStatus]);
        tw = TEXTW(stext, 0) - lrpad + 2; /* 2px right padding */
        drw_text(drw, m->ww - tw - 2 * sp, 0, tw, bh, 0, stext, 0, 0);
    }

    for (c = m->clients; c; c = c->next) {
        // prevent showing the panel as active application:
        if (ispanel(c)) continue;
        occ |= c->tags;
        if (c->isurgent)
            urg |= c->tags;
    }
    x = 0;
    for (i = 0; i < LENGTH(tags); i++) {
        /* Do not draw vacant tags */
        if (i > min_tag - 1 && !(occ & 1 << i || m->tagset[m->seltags] & 1 << i))
            continue;
        w = TEXTW(tags[i], 0);
        drw_setscheme(drw, scheme[m->tagset[m->seltags] & 1 << i ? SchemeTagsSel : SchemeTagsNorm]);
        drw_text(drw, x, 0, w, bh, lrpad / 2, tags[i], urg & 1 << i, 0);
        x += w;
    }

    w = TEXTW(m->ltsymbol, 0);
    drw_setscheme(drw, scheme[SchemeTagsNorm]);
    x = drw_text(drw, x, 0, w, bh, lrpad / 2, m->ltsymbol, 0, 0);
    if ((w = m->ww - tw - x) > bh) {
        if (m->sel) {
            drw_setscheme(drw, scheme[m == selmon ? SchemeInfoSel : SchemeInfoNorm]);
            drw_text(drw, x, 0, twidth - 2 * sp, bh, lrpad / 2, ispanel(m->sel) ? "" : m->sel->name, 0, statusfontindex);
            drw_rect(drw, x + twidth, 0, w - twidth - 2 * sp, bh, 1, 1);
            if (m->sel->isfloating && !ispanel(m->sel))
                drw_rect(drw, x + boxs, boxs, boxw, boxw, m->sel->isfixed, 0);
        } else {
            drw_setscheme(drw, scheme[SchemeInfoNorm]);
            drw_rect(drw, x, 0, w - 2 * sp, bh, 1, 1);
        }
    }
    drw_map(drw, m->barwin, 0, 0, m->ww, bh);
}

void
drawbars(void)
{
    Monitor *m;

    for (m = mons; m; m = m->next)
        drawbar(m);
}

void
drawhoverbar(Monitor *m, XMotionEvent *ev)
{
    int x, w, tw = 0;
    int boxs = drw->fonts->h / 9;
    int boxw = drw->fonts->h / 6 + 2;
    unsigned int i, occ = 0, urg = 0;
    Client *c;

    if (!m->showbar)
        return;

    //	/* draw status first so it can be overdrawn by tags later */
    if (m == selmon) {                    /* status is only drawn on selected monitor */
        drw_setscheme(drw, scheme[SchemeStatus]);
        tw = TEXTW(stext, 0) - lrpad + 2; /* 2px right padding */
        drw_text(drw, m->ww - tw - 2 * sp, 0, tw, bh, 0, stext, 0, 0);
    }

    for (c = m->clients; c; c = c->next) {
        // prevent showing the panel as active application:
        if (ispanel(c)) continue;
        occ |= c->tags;
        if (c->isurgent)
            urg |= c->tags;
    }
    x = 0;
    for (i = 0; i < LENGTH(tags); i++) {
        /* Do not draw vacant tags */
        if (i > min_tag - 1 && !(occ & 1 << i || m->tagset[m->seltags] & 1 << i))
            continue;
        w = TEXTW(tags[i], 0);
        if (m->tagset[m->seltags] & 1 << i)
            drw_setscheme(drw, scheme[SchemeTagsSel]);
        else if (ev->x > x && ev->x < x + w && (topbar ? ev->y < bh : ev->y > selmon->by))
            drw_setscheme(drw, scheme[SchemeTagsHover]);
        else
            drw_setscheme(drw, scheme[SchemeTagsNorm]);
        drw_text(drw, x, 0, w, bh, lrpad / 2, tags[i], urg & 1 << i, 0);
        x += w;
    }

    w = TEXTW(m->ltsymbol, 0);
    drw_setscheme(drw, scheme[SchemeTagsNorm]);
    x = drw_text(drw, x, 0, w, bh, lrpad / 2, m->ltsymbol, 0, 0);

    if (ev->x < x)
        XDefineCursor(dpy, selmon->barwin, cursor[CurHand]->cursor);
    else
        XDefineCursor(dpy, selmon->barwin, cursor[CurNormal]->cursor);

    if ((w = m->ww - tw - x) > bh) {
        if (m->sel) {
            drw_setscheme(drw, scheme[m == selmon ? SchemeInfoSel : SchemeInfoNorm]);
            drw_text(drw, x, 0, twidth - 2 * sp, bh, lrpad / 2, ispanel(m->sel) ? "" : m->sel->name, 0, statusfontindex);
            drw_rect(drw, x + twidth, 0, w - twidth - 2 * sp, bh, 1, 1);
            if (m->sel->isfloating && !ispanel(m->sel))
                drw_rect(drw, x + boxs, boxs, boxw, boxw, m->sel->isfixed, 0);
        } else {
            drw_setscheme(drw, scheme[SchemeInfoNorm]);
            drw_rect(drw, x, 0, w - 2 * sp, bh, 1, 1);
        }
    }
    drw_map(drw, m->barwin, 0, 0, m->ww, bh);
}

void
drawhoverbars(XMotionEvent *ev)
{
    Monitor *m;

    for (m = mons; m; m = m->next)
        drawhoverbar(m, ev);
}

void
enqueue(Client *c)
{
    Client *l;
    for (l = c->mon->clients; l && l->next; l = l->next)
        ;
    if (l) {
        l->next = c;
        c->next = NULL;
    }
}

void
enqueuestack(Client *c)
{
    Client *l;
    for (l = c->mon->stack; l && l->snext; l = l->snext)
        ;
    if (l) {
        l->snext = c;
        c->snext = NULL;
    }
}

void
enternotify(XEvent *e)
{
    Client *c;
    Monitor *m;
    XCrossingEvent *ev = &e->xcrossing;

    if ((ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root)
        return;
    c = wintoclient(ev->window);
    m = c ? c->mon : wintomon(ev->window);
    if (m != selmon) {
        unfocus(selmon->sel, 1);
        selmon = m;
    } else if (!c || c == selmon->sel)
        return;
    focus(c);
}

void
expose(XEvent *e)
{
    Monitor *m;
    XExposeEvent *ev = &e->xexpose;

    if (ev->count == 0 && (m = wintomon(ev->window)))
        drawbar(m);
}

void
unfloatexceptlatest(Monitor *m, Client *c, int action)
{

    const char *class, *instance;
    unsigned int i;
    const Rule *r;
    XClassHint ch = {NULL, NULL};
    for (i = 0; i < LENGTH(rules); i++) {
        r = &rules[i];
        switch (action) {
            case OPEN_CLIENT:
                if (c->ispreventtile)
                    return;
                for (Client *cl = m->clients; cl; cl = cl->next) {
                    XGetClassHint(dpy, cl->win, &ch);
                    class = ch.res_class ? ch.res_class : broken;
                    instance = ch.res_name ? ch.res_name : broken;
                    if (!cl->ispreventtile
                        && cl != c
                        && !ispanel(cl)
                        && cl->isfloating
                        && (!r->title || strstr(cl->name, r->title))
                        && (!r->class || strstr(class, r->class))
                        && (!r->instance || strstr(instance, r->instance)))
                        cl->isfloating ^= 1;
                }
                break;
            case CLOSE_CLIENT:
                for (Client *cl = m->clients; cl; cl = cl->next)
                    if (cl != c
                        && !ispanel(cl)
                        && cl->isfloating
                        && !cl->ispreventtile)
                        return;

                if (!c->isfloating)
                    c->isfloating ^= 1;

                XGetClassHint(dpy, c->win, &ch);
                class = ch.res_class ? ch.res_class : broken;
                instance = ch.res_name ? ch.res_name : broken;
                if (!ispanel(c)
                    && (!r->title || strstr(c->name, r->title))
                    && (!r->class || strstr(class, r->class))
                    && (!r->instance || strstr(instance, r->instance))) {
                    if (r->isfactor && !r->ispreventtile) {
                        if (r->isfactor) {
                            applyfactor(c, r);
                            XRaiseWindow(dpy, c->win);
                        }
                    }
                }
                break;
            default:
                return;
        }
    }
}

void
focus(Client *c)
{
    if (!c || !ISVISIBLE(c))
        for (c = selmon->stack; c && (!ISVISIBLE(c) || HIDDEN(c)); c = c->snext)
            ;
    if (selmon->sel && selmon->sel != c) {
        unfocus(selmon->sel, 0);

        if (selmon->hidsel) {
            hidewin(selmon->sel);
            if (c)
                arrange(c->mon);
            selmon->hidsel = 0;
        }
    }
    if (c) {
        if (c->mon != selmon)
            selmon = c->mon;
        if (c->isurgent)
            seturgent(c, 0);
        // prevents the panel getting focus when tag switching:
        if (!ispanel(c)) {
            detachstack(c);
            attachstack(c);
            grabbuttons(c, 1);
            XSetWindowBorder(dpy, c->win, scheme[SchemeSel][ColBorder].pixel);
            setfocus(c);
        }
    } else {
        // XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
        XSetInputFocus(dpy, selmon->barwin, RevertToPointerRoot, CurrentTime);
        XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
    }
    selmon->sel = c;
    drawbars();
}

/* there are some broken focus acquiring clients needing extra handling */
void
focusin(XEvent *e)
{
    XFocusChangeEvent *ev = &e->xfocus;

    if (selmon->sel && ev->window != selmon->sel->win)
        setfocus(selmon->sel);
}

void
focusmon(const Arg *arg)
{
    Monitor *m;

    if (!mons->next)
        return;
    if ((m = dirtomon(arg->i)) == selmon)
        return;
    unfocus(selmon->sel, 0);
    XWarpPointer(dpy, None, m->barwin, 0, 0, 0, 0, m->mw / 2, m->mh / 2);
    selmon = m;
    focus(NULL);
    if (selmon->sel && !ispanel(selmon->sel) && !isnotifyd(selmon->sel))
        XWarpPointer(dpy, None, selmon->sel->win, 0, 0, 0, 0, selmon->sel->w / 2, selmon->sel->h / 2);
}

void
focusstackvis(const Arg *arg)
{
    focusstack(arg->i, 0);
}

void
focusstackhid(const Arg *arg)
{
    focusstack(arg->i, 1);
}

int focussed_panel = 0; // helper for focusstack, avoids loops when panel is the only client

void
focusstack(int inc, int vis)
{
    Client *c = NULL, *i;

    // if no client selected AND exclude hiden client; if client selected but fullscreened
    if ((!selmon->sel && !vis) || (selmon->sel && selmon->sel->isfullscreen && lockfullscreen))
        return;

    if (!selmon->clients)
        return;

    if (inc > 0) {
        if (selmon->sel)
            for (c = selmon->sel->next; c && (!ISVISIBLE(c) || (!vis && HIDDEN(c))); c = c->next)
                ;
        if (!c)
            for (c = selmon->clients; c && (!ISVISIBLE(c) || (!vis && HIDDEN(c))); c = c->next)
                ;
    } else {
        if (selmon->sel) {
            for (i = selmon->clients; i != selmon->sel; i = i->next)
                if (!ispanel(i) && ISVISIBLE(i) && !(!vis && HIDDEN(i)))
                    c = i;
        } else
            c = selmon->clients;
        if (!c) {
            for (; i; i = i->next)
                if (!ispanel(i) && ISVISIBLE(i) && !(!vis && HIDDEN(i)))
                    c = i;
        }
    }
    // if is not clients and then return, to void exit dwm.
    if (!c)
        return;
    else {
        focus(c);
        restack(c->mon);
        warppointer(c->mon);
        if (HIDDEN(c)) {
            showwin(c);
            c->mon->hidsel = 1;
        }
    }

    // skipping the panel when switching focus:
    if (ispanel(c) && focussed_panel == 0) {
        focussed_panel = 1;
        focusstack(inc, 0);
        focussed_panel = 0;
    }
}

Atom
getatomprop(Client *c, Atom prop)
{
    int di;
    unsigned long dl;
    unsigned char *p = NULL;
    Atom da, atom = None;

    if (XGetWindowProperty(dpy, c->win, prop, 0L, sizeof atom, False, XA_ATOM,
                           &da, &di, &dl, &dl, &p)
            == Success
        && p) {
        atom = *(Atom *)p;
        XFree(p);
    }
    return atom;
}

int
getrootptr(int *x, int *y)
{
    int di;
    unsigned int dui;
    Window dummy;

    return XQueryPointer(dpy, root, &dummy, &dummy, x, y, &di, &di, &dui);
}

long
getstate(Window w)
{
    int format;
    long result = -1;
    unsigned char *p = NULL;
    unsigned long n, extra;
    Atom real;

    if (XGetWindowProperty(dpy, w, wmatom[WMState], 0L, 2L, False, wmatom[WMState],
                           &real, &format, &n, &extra, (unsigned char **)&p)
        != Success)
        return -1;
    if (n != 0)
        result = *p;
    XFree(p);
    return result;
}

int
gettextprop(Window w, Atom atom, char *text, unsigned int size)
{
    char **list = NULL;
    int n;
    XTextProperty name;

    if (!text || size == 0)
        return 0;
    text[0] = '\0';
    if (!XGetTextProperty(dpy, w, &name, atom) || !name.nitems)
        return 0;
    if (name.encoding == XA_STRING) {
        strncpy(text, (char *)name.value, size - 1);
    } else if (XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success && n > 0 && *list) {
        strncpy(text, *list, size - 1);
        XFreeStringList(list);
    }
    text[size - 1] = '\0';
    XFree(name.value);
    return 1;
}

void
grabbuttons(Client *c, int focused)
{
    updatenumlockmask();
    {
        unsigned int i, j;
        unsigned int modifiers[] = {0, LockMask, numlockmask, numlockmask | LockMask};
        XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
        if (!focused)
            XGrabButton(dpy, AnyButton, AnyModifier, c->win, False,
                        BUTTONMASK, GrabModeSync, GrabModeSync, None, None);
        for (i = 0; i < LENGTH(buttons); i++)
            if (buttons[i].click == ClkClientWin)
                for (j = 0; j < LENGTH(modifiers); j++)
                    XGrabButton(dpy, buttons[i].button,
                                buttons[i].mask | modifiers[j],
                                c->win, False, BUTTONMASK,
                                GrabModeAsync, GrabModeSync, None, None);
    }
}

void
grabkeys(void)
{
    updatenumlockmask();
    {
        unsigned int i, j;
        unsigned int modifiers[] = {0, LockMask, numlockmask, numlockmask | LockMask};
        KeyCode code;

        XUngrabKey(dpy, AnyKey, AnyModifier, root);
        for (i = 0; i < LENGTH(keys); i++)
            if ((code = XKeysymToKeycode(dpy, keys[i].keysym)))
                for (j = 0; j < LENGTH(modifiers); j++)
                    XGrabKey(dpy, code, keys[i].mod | modifiers[j], root,
                             True, GrabModeAsync, GrabModeAsync);
    }
}

int
ispanel(Client *c)
{
    return !strcmp(c->name, panel[0]);
}

int
isnotifyd(Client *c)
{
    return !strcmp(c->name, panel[2]);
}

void
hide(const Arg *arg)
{
    if (!selmon->sel)
        return;
    if (ispanel(selmon->sel))
        return;
    hidewin(selmon->sel);
    focus(NULL);
    arrange(selmon);
}

void
hideall(const Arg *arg)
{
    Client *c = NULL;
    for (c = selmon->clients; c; c = c->next) {
        if (ispanel(c)) continue;
        // hide clients in the current tagset
        if (c->tags == selmon->tagset[selmon->seltags])
            hidewin(c);
    }
    focus(NULL);
    arrange(selmon);
}

void
hidewin(Client *c)
{
    if (!c || HIDDEN(c))
        return;

    Window w = c->win;
    static XWindowAttributes ra, ca;

    // more or less taken directly from blackbox's hide() function
    XGrabServer(dpy);
    XGetWindowAttributes(dpy, root, &ra);
    XGetWindowAttributes(dpy, w, &ca);
    // prevent UnmapNotify events
    XSelectInput(dpy, root, ra.your_event_mask & ~SubstructureNotifyMask);
    XSelectInput(dpy, w, ca.your_event_mask & ~StructureNotifyMask);
    XUnmapWindow(dpy, w);
    setclientstate(c, IconicState);
    XSelectInput(dpy, root, ra.your_event_mask);
    XSelectInput(dpy, w, ca.your_event_mask);
    XUngrabServer(dpy);
}

void
incnmaster(const Arg *arg)
{
    selmon->nmaster = selmon->pertag->nmasters[selmon->pertag->curtag] = MAX(selmon->nmaster + arg->i, 0);
    arrange(selmon);
}

#ifdef XINERAMA
static int
isuniquegeom(XineramaScreenInfo *unique, size_t n, XineramaScreenInfo *info)
{
    while (n--)
        if (unique[n].x_org == info->x_org && unique[n].y_org == info->y_org
            && unique[n].width == info->width && unique[n].height == info->height)
            return 0;
    return 1;
}
#endif /* XINERAMA */

void
keypress(XEvent *e)
{
    unsigned int i;
    KeySym keysym;
    XKeyEvent *ev;

    ev = &e->xkey;
    keysym = XKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0);
    for (i = 0; i < LENGTH(keys); i++)
        if (keysym == keys[i].keysym
            && CLEANMASK(keys[i].mod) == CLEANMASK(ev->state)
            && keys[i].func)
            keys[i].func(&(keys[i].arg));
}

void
killclient(const Arg *arg)
{
    if (!selmon->sel)
        return;

    if (!sendevent(selmon->sel, wmatom[WMDelete])) {
        XGrabServer(dpy);
        XSetErrorHandler(xerrordummy);
        XSetCloseDownMode(dpy, DestroyAll);
        XKillClient(dpy, selmon->sel->win);
        XSync(dpy, False);
        XSetErrorHandler(xerror);
        XUngrabServer(dpy);
    }
}

void
manage(Window w, XWindowAttributes *wa)
{
    Client *c, *t = NULL;
    Window trans = None;
    XWindowChanges wc;

    c = ecalloc(1, sizeof(Client));
    c->win = w;
    /* geometry */
    c->x = c->oldx = wa->x;
    c->y = c->oldy = wa->y;
    c->w = c->oldw = wa->width;
    c->h = c->oldh = wa->height;
    c->oldbw = wa->border_width;

    updatetitle(c);
    if (XGetTransientForHint(dpy, w, &trans) && (t = wintoclient(trans))) {
        c->mon = t->mon;
        c->tags = t->tags;
    } else {
        c->mon = selmon;
        applyrules(c);
    }

    if (c->x + WIDTH(c) > c->mon->wx + c->mon->ww)
        c->x = c->mon->wx + c->mon->ww - WIDTH(c);
    if (c->y + HEIGHT(c) > c->mon->wy + c->mon->wh)
        c->y = c->mon->wy + c->mon->wh - HEIGHT(c);
    c->x = MAX(c->x, c->mon->wx);
    c->y = MAX(c->y, c->mon->wy);
    c->bw = borderpx;

    // no border - even when active
    if (ispanel(c)) c->bw = c->oldbw = 0;
    if (c->hasrulebw && !c->isfullscreen)
        wc.border_width = c->borderpx;
    else
        wc.border_width = c->bw;
    XConfigureWindow(dpy, w, CWBorderWidth, &wc);
    XSetWindowBorder(dpy, w, scheme[SchemeNorm][ColBorder].pixel);
    configure(c); /* propagates border_width, if size doesn't change */
    updatewindowtype(c);
    updatesizehints(c);
    updatewmhints(c);
    XSelectInput(dpy, w, EnterWindowMask | FocusChangeMask | PropertyChangeMask | StructureNotifyMask);
    grabbuttons(c, 0);
    if (!c->isfloating)
        c->isfloating = c->oldstate = trans != None || c->isfixed;
    if (c->isfloating) {
        unfloatexceptlatest(c->mon, c, OPEN_CLIENT);
        XRaiseWindow(dpy, c->win);
    }
    switch (attachdirection) {
        case 1:
            attachabove(c);
            break;
        case 2:
            attachaside(c);
            break;
        case 3:
            attachbelow(c);
            break;
        case 4:
            attachbottom(c);
            break;
        case 5:
            attachtop(c);
            break;
        default:
            attach(c);
    }
    attachstack(c);
    XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32, PropModeAppend,
                    (unsigned char *)&(c->win), 1);
    XMoveResizeWindow(dpy, c->win, c->x + 2 * sw, c->y, c->w, c->h); /* some windows require this */
    if (!HIDDEN(c))
        setclientstate(c, NormalState);
    if (c->mon == selmon)
        unfocus(selmon->sel, 0);
    c->mon->sel = c;
    arrange(c->mon);
    if (!HIDDEN(c))
        XMapWindow(dpy, c->win);
    warppointer(c->mon);
    focus(NULL);
}

void
mappingnotify(XEvent *e)
{
    XMappingEvent *ev = &e->xmapping;

    XRefreshKeyboardMapping(ev);
    if (ev->request == MappingKeyboard)
        grabkeys();
}

void
maprequest(XEvent *e)
{
    static XWindowAttributes wa;
    XMapRequestEvent *ev = &e->xmaprequest;

    if (!XGetWindowAttributes(dpy, ev->window, &wa) || wa.override_redirect)
        return;
    if (!wintoclient(ev->window))
        manage(ev->window, &wa);
}

void
monocle(Monitor *m)
{
    unsigned int n = 0;
    Client *c;
    for (c = m->clients; c; c = c->next)
        if (ISVISIBLE(c))
            n++;
    if (n > 0) /* override layout symbol */
        snprintf(m->ltsymbol, sizeof m->ltsymbol, "%s", "󰬔");
    for (c = nexttiled(m->clients); c; c = nexttiled(c->next))
        resize(c, m->wx + m->gappx, m->wy + m->gappx, m->ww - 2 * c->bw - (2 * m->gappx), m->wh - 2 * c->bw - (2 * m->gappx), 0);
}

void
motionnotify(XEvent *e)
{
    static Monitor *mon = NULL;
    Monitor *m;
    XMotionEvent *ev = &e->xmotion;

    if (ev->window != root)
        return;

    drawhoverbars(ev);

    if ((m = recttomon(ev->x_root, ev->y_root, 1, 1)) != mon && mon) {
        unfocus(selmon->sel, 1);
        selmon = m;
        focus(NULL);
    }

    mon = m;
}

void
movemouse(const Arg *arg)
{
    int x, y, ocx, ocy, nx, ny;
    Client *c;
    Monitor *m;
    XEvent ev;
    Time lasttime = 0;

    if (!(c = selmon->sel))
        return;
    restack(selmon);
    ocx = c->x;
    ocy = c->y;
    if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
                     None, cursor[CurMove]->cursor, CurrentTime)
        != GrabSuccess)
        return;
    if (!getrootptr(&x, &y))
        return;
    do {
        XMaskEvent(dpy, MOUSEMASK | ExposureMask | SubstructureRedirectMask, &ev);
        switch (ev.type) {
            case ConfigureRequest:
            case Expose:
            case MapRequest:
                handler[ev.type](&ev);
                break;
            case MotionNotify:
                if ((ev.xmotion.time - lasttime) <= (1000 / 60))
                    continue;
                lasttime = ev.xmotion.time;

                nx = ocx + (ev.xmotion.x - x);
                ny = ocy + (ev.xmotion.y - y);
                if (abs(selmon->wx - nx) < snap)
                    nx = selmon->wx;
                else if (abs((selmon->wx + selmon->ww) - (nx + WIDTH(c))) < snap)
                    nx = selmon->wx + selmon->ww - WIDTH(c);
                if (abs(selmon->wy - ny) < snap)
                    ny = selmon->wy;
                else if (abs((selmon->wy + selmon->wh) - (ny + HEIGHT(c))) < snap)
                    ny = selmon->wy + selmon->wh - HEIGHT(c);
                if (!selmon->lt[selmon->sellt]->arrange || c->isfloating)
                    resize(c, nx, ny, c->w, c->h, 1);
                else if (selmon->lt[selmon->sellt]->arrange || !c->isfloating) {
                    if ((m = recttomon(ev.xmotion.x_root, ev.xmotion.y_root, 1, 1)) != selmon) {
                        sendmon(c, m);
                        selmon = m;
                        focus(NULL);
                    }

                    Client *cc = c->mon->clients;
                    while (1) {
                        if (cc == 0) break;
                        if (
                            cc != c && !cc->isfloating && ISVISIBLE(cc) && ev.xmotion.x_root > cc->x && ev.xmotion.x_root < cc->x + cc->w && ev.xmotion.y_root > cc->y && ev.xmotion.y_root < cc->y + cc->h)
                            break;

                        cc = cc->next;
                    }

                    if (cc) {
                        Client *cl1, *cl2, ocl1;

                        if (!selmon->lt[selmon->sellt]->arrange) return;

                        cl1 = c;
                        cl2 = cc;
                        ocl1 = *cl1;
                        strcpy(cl1->name, cl2->name);
                        cl1->win = cl2->win;
                        cl1->x = cl2->x;
                        cl1->y = cl2->y;
                        cl1->w = cl2->w;
                        cl1->h = cl2->h;

                        cl2->win = ocl1.win;
                        strcpy(cl2->name, ocl1.name);
                        cl2->x = ocl1.x;
                        cl2->y = ocl1.y;
                        cl2->w = ocl1.w;
                        cl2->h = ocl1.h;

                        selmon->sel = cl2;

                        c = cc;
                        focus(c);

                        arrange(cl1->mon);
                    }
                }
                break;
        }
    } while (ev.type != ButtonRelease);
    XUngrabPointer(dpy, CurrentTime);
    if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
        sendmon(c, m);
        selmon = m;
        focus(NULL);
    }
}

Client *
nexttagged(Client *c)
{
    Client *walked = c->mon->clients;
    for (;
         walked && (walked->isfloating || !ISVISIBLEONTAG(walked, c->tags));
         walked = walked->next)
        ;
    return walked;
}

void
movethrow(const Arg *arg)
{
    Client *c;
    int nh, nw, nx, ny;
    c = selmon->sel;
    if (selmon->lt[selmon->sellt]->arrange && !c->isfloating)
        togglefloating(NULL);
    nw = c->w;
    nh = c->h;
    switch (arg->ui) {
        case WIN_N:
            nx = c->x;
            ny = selmon->wy + selmon->gappx;
            break;
        case WIN_E:
            nx = selmon->wx + selmon->ww - c->w - c->bw * 2 - selmon->gappx;
            ny = c->y;
            break;
        case WIN_S:
            nx = c->x;
            ny = selmon->wy + selmon->wh - c->h - c->bw * 2 - selmon->gappx;
            break;
        case WIN_W:
            nx = selmon->wx + selmon->gappx;
            ny = c->y;
            break;
        case WIN_C:
            nx = selmon->wx + ((selmon->ww - c->w - c->bw * 2) / 2);
            ny = selmon->wy + ((selmon->wh - c->h - c->bw * 2) / 2);
            break;
        default:
            return;
    }
    resize(c, nx, ny, nw, nh, True);
    XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, nw / 2, nh / 2);
}

Client *
nexttiled(Client *c)
{
    for (; c && (c->isfloating || !ISVISIBLE(c) || HIDDEN(c)); c = c->next)
        ;
    return c;
}

void
pop(Client *c)
{
    detach(c);
    attach(c);
    focus(c);
    arrange(c->mon);
}

void
propertynotify(XEvent *e)
{
    Client *c;
    Window trans;
    XPropertyEvent *ev = &e->xproperty;

    if ((ev->window == root) && (ev->atom == XA_WM_NAME))
        updatestatus();
    else if (ev->state == PropertyDelete)
        return; /* ignore */
    else if ((c = wintoclient(ev->window))) {
        switch (ev->atom) {
            default:
                break;
            case XA_WM_TRANSIENT_FOR:
                if (!c->isfloating && (XGetTransientForHint(dpy, c->win, &trans)) && (c->isfloating = (wintoclient(trans)) != NULL))
                    arrange(c->mon);
                break;
            case XA_WM_NORMAL_HINTS:
                c->hintsvalid = 0;
                break;
            case XA_WM_HINTS:
                updatewmhints(c);
                drawbars();
                break;
        }
        if (ev->atom == XA_WM_NAME || ev->atom == netatom[NetWMName]) {
            updatetitle(c);
            if (c == c->mon->sel)
                drawbar(c->mon);
        }
        if (ev->atom == netatom[NetWMWindowType])
            updatewindowtype(c);
    }
}

void
quit(const Arg *arg)
{
    FILE *fd = NULL;
    struct stat filestat;

    if ((fd = fopen(lockfile, "r")) && stat(lockfile, &filestat) == 0) {
        fclose(fd);

        if (filestat.st_ctime <= time(NULL) - 2)
            remove(lockfile);
    }

    if ((fd = fopen(lockfile, "r")) != NULL) {
        fclose(fd);
        remove(lockfile);
        // fix: reloading dwm keeps all the hidden clients hidden
        Monitor *m;
        Client *c;
        for (m = mons; m; m = m->next) {
            if (m) {
                for (c = m->stack; c; c = c->next)
                    if (c && HIDDEN(c)) showwin(c);
            }
        }
        running = 0;
    } else {
        if ((fd = fopen(lockfile, "a")) != NULL)
            fclose(fd);
    }
}

Monitor *
recttomon(int x, int y, int w, int h)
{
    Monitor *m, *r = selmon;
    int a, area = 0;

    for (m = mons; m; m = m->next)
        if ((a = INTERSECT(x, y, w, h, m)) > area) {
            area = a;
            r = m;
        }
    return r;
}

void
resetnmaster(const Arg *arg)
{
    selmon->nmaster = 1;
    arrange(selmon);
}

void
resize(Client *c, int x, int y, int w, int h, int interact)
{
    if (ispanel(c) || applysizehints(c, &x, &y, &w, &h, interact))
        resizeclient(c, x, y, w, h);
}

void
resizebarwin(Monitor *m)
{
    unsigned int w = m->ww;
    XMoveResizeWindow(dpy, m->barwin, m->wx + sp, m->by + vp, w - 2 * sp, bh);
}

void
resizeclient(Client *c, int x, int y, int w, int h)
{
    XWindowChanges wc;

    c->oldx = c->x;
    c->x = wc.x = x;
    c->oldy = c->y;
    c->y = wc.y = y;
    c->oldw = c->w;
    c->w = wc.width = w;
    c->oldh = c->h;
    c->h = wc.height = h;
    if (c->hasrulebw && !c->isfullscreen)
        wc.border_width = c->borderpx;
    else
        wc.border_width = c->bw;
    // nail it to no border & y=0:
    if (((nexttiled(c->mon->clients) == c && !nexttiled(c->next))
         || &monocle == c->mon->lt[c->mon->sellt]->arrange)
        && !c->isfullscreen && !c->isfloating
        && NULL != c->mon->lt[c->mon->sellt]->arrange) {
        c->w = wc.width += c->bw * 2;
        c->h = wc.height += c->bw * 2;
        wc.border_width = 0;
    }
    if (ispanel(c)) c->y = c->oldy = c->bw = wc.y = wc.border_width = 0;
    XConfigureWindow(dpy, c->win, CWX | CWY | CWWidth | CWHeight | CWBorderWidth, &wc);
    configure(c);
    XSync(dpy, False);
}

void
resizemouse(const Arg *arg)
{
    int ocx, ocy, nw, nh;
    int ocx2, ocy2, nx, ny;
    Client *c;
    Monitor *m;
    XEvent ev;
    int horizcorner, vertcorner;
    int di;
    unsigned int dui;
    Window dummy;
    Time lasttime = 0;

    if (!(c = selmon->sel))
        return;
    restack(selmon);
    ocx = c->x;
    ocy = c->y;
    ocx2 = c->x + c->w;
    ocy2 = c->y + c->h;
    if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
                     None, cursor[CurResize]->cursor, CurrentTime)
        != GrabSuccess)
        return;
    if (!XQueryPointer(dpy, c->win, &dummy, &dummy, &di, &di, &nx, &ny, &dui))
        return;
    horizcorner = nx < c->w / 2;
    vertcorner = ny < c->h / 2;
    XWarpPointer(dpy, None, c->win, 0, 0, 0, 0,
                 horizcorner ? (-c->bw) : (c->w + c->bw - 1),
                 vertcorner ? (-c->bw) : (c->h + c->bw - 1));
    do {
        XMaskEvent(dpy, MOUSEMASK | ExposureMask | SubstructureRedirectMask, &ev);
        switch (ev.type) {
            case ConfigureRequest:
            case Expose:
            case MapRequest:
                handler[ev.type](&ev);
                break;
            case MotionNotify:
                if ((ev.xmotion.time - lasttime) <= (1000 / 60))
                    continue;
                lasttime = ev.xmotion.time;

                nx = horizcorner ? ev.xmotion.x : c->x;
                ny = vertcorner ? ev.xmotion.y : c->y;
                nw = MAX(horizcorner ? (ocx2 - nx) : (ev.xmotion.x - ocx - 2 * c->bw + 1), 1);
                nh = MAX(vertcorner ? (ocy2 - ny) : (ev.xmotion.y - ocy - 2 * c->bw + 1), 1);

                if (c->mon->wx + nw >= selmon->wx && c->mon->wx + nw <= selmon->wx + selmon->ww
                    && c->mon->wy + nh >= selmon->wy && c->mon->wy + nh <= selmon->wy + selmon->wh) {
                    if (!c->isfloating && selmon->lt[selmon->sellt]->arrange
                        && (abs(nw - c->w) > snap || abs(nh - c->h) > snap))
                        togglefloating(NULL);
                }
                if (!selmon->lt[selmon->sellt]->arrange || c->isfloating)
                    resize(c, nx, ny, nw, nh, 1);
                break;
        }
    } while (ev.type != ButtonRelease);
    XWarpPointer(dpy, None, c->win, 0, 0, 0, 0,
                 horizcorner ? (-c->bw) : (c->w + c->bw - 1),
                 vertcorner ? (-c->bw) : (c->h + c->bw - 1));
    XUngrabPointer(dpy, CurrentTime);
    while (XCheckMaskEvent(dpy, EnterWindowMask, &ev))
        ;
    if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
        sendmon(c, m);
        selmon = m;
        focus(NULL);
    }
}

void
restack(Monitor *m)
{
    Client *c;
    XEvent ev;
    XWindowChanges wc;

    drawbar(m);
    if (!m->sel)
        return;

    if (m->sel->isfloating || !m->lt[m->sellt]->arrange)
        XRaiseWindow(dpy, m->sel->win);

    if (m->sel->isfloating && m->sel->isbehide)
        XLowerWindow(dpy, m->sel->win);

    if (m->lt[m->sellt]->arrange) {
        wc.stack_mode = Below;
        wc.sibling = m->barwin;
        for (c = m->stack; c; c = c->snext)
            if (!c->isfloating && ISVISIBLE(c)) {
                XConfigureWindow(dpy, c->win, CWSibling | CWStackMode, &wc);
                wc.sibling = c->win;
            }
    }
    XSync(dpy, False);
    while (XCheckMaskEvent(dpy, EnterWindowMask, &ev))
        ;
}

void
rotatestack(const Arg *arg)
{
    Client *c = NULL, *f;

    if (!selmon->sel)
        return;
    f = selmon->sel;
    if (arg->i > 0) {
        for (c = nexttiled(selmon->clients); c && nexttiled(c->next); c = nexttiled(c->next))
            ;
        if (c) {
            detach(c);
            attach(c);
            detachstack(c);
            attachstack(c);
        }
    } else if ((c = nexttiled(selmon->clients))) {
        detach(c);
        enqueue(c);
        detachstack(c);
        enqueuestack(c);
    }
    if (c) {
        arrange(selmon);
        // unfocus(f, 1);
        focus(f);
        warppointer(selmon);
        restack(selmon);
    }
}

void
run(void)
{
    XEvent ev;
    /* main event loop */
    XSync(dpy, False);
    while (running && !XNextEvent(dpy, &ev))
        if (handler[ev.type])
            handler[ev.type](&ev); /* call handler */
}

void
runautostart(void)
{
    char *pathpfx;
    char *path;
    char *xdgdatahome;
    char *home;
    struct stat sb;

    if ((home = getenv("HOME")) == NULL)
        /* this is almost impossible */
        return;

    /* if $XDG_DATA_HOME is set and not empty, use $XDG_DATA_HOME/dwm,
     * otherwise use ~/.local/share/dwm as autostart script directory
     */
    xdgdatahome = getenv("XDG_DATA_HOME");
    if (xdgdatahome != NULL && *xdgdatahome != '\0') {
        /* space for path segments, separators and nul */
        pathpfx = ecalloc(1, strlen(xdgdatahome) + strlen(dwmdir) + 2);

        if (sprintf(pathpfx, "%s/%s", xdgdatahome, dwmdir) <= 0) {
            free(pathpfx);
            return;
        }
    } else {
        /* space for path segments, separators and nul */
        pathpfx = ecalloc(1, strlen(home) + strlen(localshare)
                                 + strlen(dwmdir) + 3);

        if (sprintf(pathpfx, "%s/%s/%s", home, localshare, dwmdir) < 0) {
            free(pathpfx);
            return;
        }
    }

    /* check if the autostart script directory exists */
    if (!(stat(pathpfx, &sb) == 0 && S_ISDIR(sb.st_mode))) {
        /* the XDG conformant path does not exist or is no directory
         * so we try ~/.dwm instead
         */
        char *pathpfx_new = realloc(pathpfx, strlen(home) + strlen(dwmdir) + 3);
        if (pathpfx_new == NULL) {
            free(pathpfx);
            return;
        }
        pathpfx = pathpfx_new;

        if (sprintf(pathpfx, "%s/.%s", home, dwmdir) <= 0) {
            free(pathpfx);
            return;
        }
    }

    /* try the blocking script first */
    path = ecalloc(1, strlen(pathpfx) + strlen(autostartblocksh) + 2);
    if (sprintf(path, "%s/%s", pathpfx, autostartblocksh) <= 0) {
        free(path);
        free(pathpfx);
    }

    if (access(path, X_OK) == 0)
        system(path);

    /* now the non-blocking script */
    if (sprintf(path, "%s/%s", pathpfx, autostartsh) <= 0) {
        free(path);
        free(pathpfx);
    }

    if (access(path, X_OK) == 0)
        system(strcat(path, " &"));

    free(pathpfx);
    free(path);
}

void
scan(void)
{
    unsigned int i, num;
    Window d1, d2, *wins = NULL;
    XWindowAttributes wa;

    if (XQueryTree(dpy, root, &d1, &d2, &wins, &num)) {
        for (i = 0; i < num; i++) {
            if (!XGetWindowAttributes(dpy, wins[i], &wa)
                || wa.override_redirect || XGetTransientForHint(dpy, wins[i], &d1))
                continue;
            if (wa.map_state == IsViewable || getstate(wins[i]) == IconicState)
                manage(wins[i], &wa);
        }
        for (i = 0; i < num; i++) { /* now the transients */
            if (!XGetWindowAttributes(dpy, wins[i], &wa))
                continue;
            if (XGetTransientForHint(dpy, wins[i], &d1)
                && (wa.map_state == IsViewable || getstate(wins[i]) == IconicState))
                manage(wins[i], &wa);
        }
        if (wins)
            XFree(wins);
    }
}

void
sendmon(Client *c, Monitor *m)
{
    if (c->mon == m)
        return;

    if (!c)
        return;

    if (ispanel(c))
        return;

    unfocus(c, 1);
    detach(c);
    detachstack(c);
    c->mon = m;
    c->tags = m->tagset[m->seltags]; /* assign tags of target monitor */
    switch (attachdirection) {
        case 1:
            attachabove(c);
            break;
        case 2:
            attachaside(c);
            break;
        case 3:
            attachbelow(c);
            break;
        case 4:
            attachbottom(c);
            break;
        case 5:
            attachtop(c);
            break;
        default:
            attach(c);
    }
    attachstack(c);
    focus(NULL);
    arrange(NULL);
}

void
setclientstate(Client *c, long state)
{
    long data[] = {state, None};

    XChangeProperty(dpy, c->win, wmatom[WMState], wmatom[WMState], 32,
                    PropModeReplace, (unsigned char *)data, 2);
}

int
sendevent(Client *c, Atom proto)
{
    int n;
    Atom *protocols;
    int exists = 0;
    XEvent ev;

    if (XGetWMProtocols(dpy, c->win, &protocols, &n)) {
        while (!exists && n--)
            exists = protocols[n] == proto;
        XFree(protocols);
    }

    if (exists) {
        ev.type = ClientMessage;
        ev.xclient.window = c->win;
        ev.xclient.message_type = wmatom[WMProtocols];
        ev.xclient.format = 32;
        ev.xclient.data.l[0] = proto;
        ev.xclient.data.l[1] = CurrentTime;
        XSendEvent(dpy, c->win, False, NoEventMask, &ev);
    }
    return exists;
}

void
setfocus(Client *c)
{
    if (!c->neverfocus) {
        XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
        XChangeProperty(dpy, root, netatom[NetActiveWindow],
                        XA_WINDOW, 32, PropModeReplace,
                        (unsigned char *)&(c->win), 1);
    }
    sendevent(c, wmatom[WMTakeFocus]);
}

void
setfullscreen(Client *c, int fullscreen)
{
    if (fullscreen && !c->isfullscreen) {
        XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
                        PropModeReplace, (unsigned char *)&netatom[NetWMFullscreen], 1);
        c->isfullscreen = 1;
    } else if (!fullscreen && c->isfullscreen) {
        XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
                        PropModeReplace, (unsigned char *)0, 0);
        c->isfullscreen = 0;
    }
}

void
setgaps(const Arg *arg)
{
    if ((arg->i == 0) || (selmon->gappx + arg->i < 0))
        selmon->gappx = 0;
    else
        selmon->gappx += arg->i;
    arrange(selmon);
}

void
setlayout(const Arg *arg)
{
    if (!arg || !arg->v || arg->v != selmon->lt[selmon->sellt]) {
        selmon->pertag->sellts[selmon->pertag->curtag] ^= 1;
        selmon->sellt = selmon->pertag->sellts[selmon->pertag->curtag];
    }
    if (arg && arg->v)
        selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt] = (Layout *)arg->v;
    selmon->lt[selmon->sellt] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt];
    strncpy(selmon->ltsymbol, selmon->lt[selmon->sellt]->symbol, sizeof selmon->ltsymbol);
    if (selmon->sel)
        arrange(selmon);
    else
        drawbar(selmon);
}

/* arg > 1.0 will set mfact absolutely */
void
setmfact(const Arg *arg)
{
    float f;

    if (!arg || !selmon->lt[selmon->sellt]->arrange)
        return;
    f = arg->f < 1.0 ? arg->f + selmon->mfact : arg->f - 1.0;
    if (f < 0.05 || f > 0.95)
        return;
    selmon->mfact = selmon->pertag->mfacts[selmon->pertag->curtag] = f;
    arrange(selmon);
}

void
setup(void)
{
    int i;
    XSetWindowAttributes wa;
    Atom utf8string;

    /* clean up any zombies immediately */
    sigchld(0);

    /* init screen */
    screen = DefaultScreen(dpy);
    sw = DisplayWidth(dpy, screen);
    sh = DisplayHeight(dpy, screen);
    root = RootWindow(dpy, screen);
    xinitvisual();
    drw = drw_create(dpy, screen, root, sw, sh, visual, depth, cmap);
    if (!drw_fontset_create(drw, fonts, LENGTH(fonts)))
        die("no fonts could be loaded.");
    lrpad = drw->fonts->h;
    bh = user_bh ? user_bh : drw->fonts->h + 2;
    updategeom();
    sp = sidepad;
    vp = (topbar == 1) ? vertpad : -vertpad;

    /* init atoms */
    utf8string = XInternAtom(dpy, "UTF8_STRING", False);
    wmatom[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
    wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    wmatom[WMState] = XInternAtom(dpy, "WM_STATE", False);
    wmatom[WMTakeFocus] = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
    netatom[NetActiveWindow] = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
    netatom[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", False);
    netatom[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
    netatom[NetWMState] = XInternAtom(dpy, "_NET_WM_STATE", False);
    netatom[NetWMCheck] = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
    netatom[NetWMFullscreen] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
    netatom[NetWMWindowType] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
    netatom[NetWMWindowTypeDialog] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
    netatom[NetClientList] = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
    /* init cursors */
    cursor[CurNormal] = drw_cur_create(drw, XC_left_ptr);
    cursor[CurResize] = drw_cur_create(drw, XC_sizing);
    cursor[CurMove] = drw_cur_create(drw, XC_fleur);
    cursor[CurHand] = drw_cur_create(drw, XC_hand2);
    /* init appearance */
    scheme = ecalloc(LENGTH(colors), sizeof(Clr *));
    for (i = 0; i < LENGTH(colors); i++)
        scheme[i] = drw_scm_create(drw, colors[i], alphas[i], 3);
    /* init bars */
    updatebars();
    updatestatus();
    updatebarpos(selmon);
    /* supporting window for NetWMCheck */
    wmcheckwin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
    XChangeProperty(dpy, wmcheckwin, netatom[NetWMCheck], XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *)&wmcheckwin, 1);
    XChangeProperty(dpy, wmcheckwin, netatom[NetWMName], utf8string, 8,
                    PropModeReplace, (unsigned char *)"dwm", 3);
    XChangeProperty(dpy, root, netatom[NetWMCheck], XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *)&wmcheckwin, 1);
    /* EWMH support per view */
    XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)netatom, NetLast);
    XDeleteProperty(dpy, root, netatom[NetClientList]);
    /* select events */
    wa.cursor = cursor[CurNormal]->cursor;
    wa.event_mask = SubstructureRedirectMask | SubstructureNotifyMask
                  | ButtonPressMask | PointerMotionMask | EnterWindowMask
                  | LeaveWindowMask | StructureNotifyMask | PropertyChangeMask;
    XChangeWindowAttributes(dpy, root, CWEventMask | CWCursor, &wa);
    XSelectInput(dpy, root, wa.event_mask);
    grabkeys();
    focus(NULL);
}

void
seturgent(Client *c, int urg)
{
    XWMHints *wmh;

    c->isurgent = urg;
    if (!(wmh = XGetWMHints(dpy, c->win)))
        return;
    wmh->flags = urg ? (wmh->flags | XUrgencyHint) : (wmh->flags & ~XUrgencyHint);
    XSetWMHints(dpy, c->win, wmh);
    XFree(wmh);
}

void
show(const Arg *arg)
{
    if (selmon->hidsel)
        selmon->hidsel = 0;
    showwin(selmon->sel);
}

void
showall(const Arg *arg)
{
    Client *c = NULL;
    selmon->hidsel = 0;
    for (c = selmon->clients; c; c = c->next)
        if (ISVISIBLE(c))
            showwin(c);
    if (!selmon->sel) {
        for (c = selmon->clients; c && !ISVISIBLE(c); c = c->next)
            ;
        if (c)
            focus(c);
    }
    restack(selmon);
}

void
showwin(Client *c)
{
    if (!c || !HIDDEN(c))
        return;

    XMapWindow(dpy, c->win);
    setclientstate(c, NormalState);
    arrange(c->mon);
}

void
showhide(Client *c)
{
    if (!c)
        return;
    if (ISVISIBLE(c)) {
        /* show clients top down */
        XMoveWindow(dpy, c->win, c->x, c->y);
        if (!c->mon->lt[c->mon->sellt]->arrange || c->isfloating)
            resize(c, c->x, c->y, c->w, c->h, 0);
        showhide(c->snext);
    } else {
        /* hide clients bottom up */
        showhide(c->snext);
        XMoveWindow(dpy, c->win, WIDTH(c) * -2, c->y);
    }
}

void
sigchld(int unused)
{
    if (signal(SIGCHLD, sigchld) == SIG_ERR)
        die("can't install SIGCHLD handler:");
    while (0 < waitpid(-1, NULL, WNOHANG))
        ;
}

void
spawn(const Arg *arg)
{
    if (fork() == 0) {
        if (dpy)
            close(ConnectionNumber(dpy));
        setsid();
        execvp(((char **)arg->v)[0], (char **)arg->v);
        die("dwm: execvp '%s' failed:", ((char **)arg->v)[0]);
    }
}

void
tag(const Arg *arg)
{
    if (selmon->sel && arg->ui & TAGMASK) {
        selmon->sel->tags = arg->ui & TAGMASK;
        focus(NULL);
        arrange(selmon);
        if (viewontag && ((arg->ui & TAGMASK) != TAGMASK))
            view(arg);
    }
}

void
tagmon(const Arg *arg)
{
    Monitor *m;
    Client *c = selmon->sel;
    unsigned int source, destination, primary;

    if (!c || !mons->next || !entagmon)
        return;

    source = c->mon->num;
    sendmon(c, dirtomon(arg->i));
    focusmon(arg);
    destination = c->mon->num;
    primary = !selmon->num ? 0 : !selmon->num;
    // get primary monitor
    for (m = mons; m->num; m = m->next)
        ;
    if (source == primary
        && source != destination
        && destination != primary && c->isfloating) {
        dotogglefloating(c->mon, c);
        arrange(c->mon);
        if (isfloating_src) {
            // primany --> displayPort-0
            if (m->clients) {
                Client *cl = m->clients;
                while (cl->next && !ispanel(cl->next))
                    cl = cl->next;
                unfloatexceptlatest(cl->mon, cl, CLOSE_CLIENT);
            }
            arrange(m);
        }
    } else if (source != primary
               && source != destination
               && destination == primary && !c->isfloating) {
        // displayPort-0 --> primany
        if (isfloating_src) {
            c->isfloating ^= 1;
            changerule(c);
            XRaiseWindow(dpy, c->win);
            unfloatexceptlatest(m, c, OPEN_CLIENT);
            arrange(m);
        }
    }
}

void
tile(Monitor *m)
{
    unsigned int i, n, h, g = 0, mw, my, ty;
    Client *c;

    for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++)
        ;
    if (n == 0)
        return;

    if (n > m->nmaster)
        mw = m->nmaster
               ? (m->ww - (g = gappx)) * (m->rmaster ? 1.0 - m->mfact : m->mfact)
               : 0;
    else
        mw = m->ww - m->gappx;
    for (i = 0, my = ty = m->gappx, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
        if (i < m->nmaster) {
            h = (m->wh - my) / (MIN(n, m->nmaster) - i) - m->gappx;
            resize(c, m->rmaster ? (m->wx + m->ww - mw) : (m->wx + m->gappx), m->wy + my, mw - (2 * c->bw) - m->gappx, h - (2 * c->bw), 0);
            if (my + HEIGHT(c) + m->gappx < m->wh)
                my += HEIGHT(c) + m->gappx;
        } else {
            h = (m->wh - ty) / (n - i) - m->gappx;
            resize(c, m->rmaster ? (m->wx + m->gappx) : (m->wx + mw + m->gappx), m->wy + ty, m->ww - mw - (2 * c->bw) - 2 * m->gappx, h - (2 * c->bw), 0);
            if (ty + HEIGHT(c) + m->gappx < m->wh)
                ty += HEIGHT(c) + m->gappx;
        }
}

void
togglebar(const Arg *arg)
{
    Client *c;
    selmon->showbar = !selmon->showbar;
    for (c = selmon->clients; c; c = c->next) {
        if (!ispanel(c))
            continue;
        selmon->showbar ? showwin(c) : hidewin(c);
    }
    updatebarpos(selmon);
    resizebarwin(selmon);
    arrange(selmon);
}

void
togglefloating(const Arg *arg)
{
    Monitor *m = selmon;
    Client *c = m->sel;
    dotogglefloating(m, c);
    isfloating_src = c->isfloating;
    arrange(m);
    warppointer(m);
}

void
togglebehide(const Arg *arg)
{
    if (!selmon->sel)
        return;
    if (selmon->sel->isfullscreen)
        return;
    if (!ISVISIBLE(selmon->sel))
        return;
    if (ispanel(selmon->sel))
        return;
    if (selmon->sel->isfloating)
        selmon->sel->isbehide ^= 1;
    focus(NULL);
    arrange(selmon);
}

void
togglermaster(const Arg *arg)
{
    selmon->rmaster = !selmon->rmaster;
    /* now mfact represents the left factor */
    selmon->mfact = 1.0 - selmon->mfact;
    if (selmon->lt[selmon->sellt]->arrange)
        arrange(selmon);
    warppointer(selmon);
}

void
toggletag(const Arg *arg)
{
    unsigned int newtags;

    if (!selmon->sel)
        return;
    newtags = selmon->sel->tags ^ (arg->ui & TAGMASK);
    if (newtags) {
        selmon->sel->tags = newtags;
        focus(NULL);
        arrange(selmon);
    }
}

void
toggleview(const Arg *arg)
{
    unsigned int newtagset = selmon->tagset[selmon->seltags] ^ (arg->ui & TAGMASK);
    int i;
    if (newtagset) {
        if (newtagset == ~0) {
            selmon->pertag->prevtag = selmon->pertag->curtag;
            selmon->pertag->curtag = 0;
        }
        /* test if the user did not select the same tag */
        if (!(newtagset & 1 << (selmon->pertag->curtag - 1))) {
            selmon->pertag->prevtag = selmon->pertag->curtag;
            for (i = 0; !(newtagset & 1 << i); i++)
                ;
            selmon->pertag->curtag = i + 1;
        }
        selmon->tagset[selmon->seltags] = newtagset;

        /* apply settings for this view */
        selmon->nmaster = selmon->pertag->nmasters[selmon->pertag->curtag];
        selmon->mfact = selmon->pertag->mfacts[selmon->pertag->curtag];
        selmon->sellt = selmon->pertag->sellts[selmon->pertag->curtag];
        selmon->lt[selmon->sellt] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt];
        selmon->lt[selmon->sellt ^ 1] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt ^ 1];
        focus(NULL);
        arrange(selmon);
    }
}

void
unfocus(Client *c, int setfocus)
{
    if (!c)
        return;
    grabbuttons(c, 0);
    XSetWindowBorder(dpy, c->win, scheme[SchemeNorm][ColBorder].pixel);
    if (setfocus) {
        XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
        XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
    }
}

void
unmanage(Client *c, int destroyed)
{
    Monitor *m = c->mon;
    XWindowChanges wc;
    int ispreventtile = c->ispreventtile;
    int isfloating = c->isfloating;
    detach(c);
    detachstack(c);
    if (!destroyed) {
        wc.border_width = c->oldbw;
        XGrabServer(dpy); /* avoid race conditions */
        XSetErrorHandler(xerrordummy);
        XSelectInput(dpy, c->win, NoEventMask);
        XConfigureWindow(dpy, c->win, CWBorderWidth, &wc); /* restore border */
        XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
        setclientstate(c, WithdrawnState);
        XSync(dpy, False);
        XSetErrorHandler(xerror);
        XUngrabServer(dpy);
    }
    free(c);
    if (m->clients) {
        Client *cl = m->clients;
        while (cl->next && !ispanel(cl->next))
            cl = cl->next;
        if (!ispreventtile && isfloating) {
            unfloatexceptlatest(m, cl, CLOSE_CLIENT);
            if (cl->isfloating)
                XRaiseWindow(dpy, cl->win);
        }
    }
    focus(NULL);
    updateclientlist();
    arrange(m);
    if (m == selmon && selmon->sel && !isfloating)
        warppointer(m);
}

void
unmapnotify(XEvent *e)
{
    Client *c;
    XUnmapEvent *ev = &e->xunmap;

    if ((c = wintoclient(ev->window))) {
        if (ev->send_event)
            setclientstate(c, WithdrawnState);
        else
            unmanage(c, 0);
    }
}

void
updatebars(void)
{
    Monitor *m;
    XSetWindowAttributes wa = {
        .override_redirect = True,
        .background_pixel = 0,
        .border_pixel = 0,
        .colormap = cmap,
        .event_mask = ButtonPressMask | ExposureMask};
    XClassHint ch = {"dwm", "dwm"};
    for (m = mons; m; m = m->next) {
        if (m->barwin)
            continue;
        m->barwin = XCreateWindow(dpy, root, m->wx + sp, m->by + vp, m->ww - 2 * sp, bh, 0, depth,
                                  InputOutput, visual,
                                  CWOverrideRedirect | CWBackPixel | CWBorderPixel | CWColormap | CWEventMask, &wa);
        XDefineCursor(dpy, m->barwin, cursor[CurNormal]->cursor);
        XMapRaised(dpy, m->barwin);
        XSetClassHint(dpy, m->barwin, &ch);
    }
}

void
updatebarpos(Monitor *m)
{
    m->wy = m->my;
    m->wh = m->mh;
    if (!m->num && m->showbar) { // only show bar on first monitor
        m->wh = m->wh - vertpad - bh;
        m->by = m->topbar ? m->wy : m->wy + m->wh + vertpad;
        m->wy = m->topbar ? m->wy + bh + vp : m->wy;
    } else
        m->by = -bh - vp;
}

void
updateclientlist()
{
    Client *c;
    Monitor *m;

    XDeleteProperty(dpy, root, netatom[NetClientList]);
    for (m = mons; m; m = m->next)
        for (c = m->clients; c; c = c->next)
            XChangeProperty(dpy, root, netatom[NetClientList],
                            XA_WINDOW, 32, PropModeAppend,
                            (unsigned char *)&(c->win), 1);
}

int
updategeom(void)
{
    int dirty = 0;

#ifdef XINERAMA
    if (XineramaIsActive(dpy)) {
        int i, j, n, nn;
        Client *c;
        Monitor *m;
        XineramaScreenInfo *info = XineramaQueryScreens(dpy, &nn);
        XineramaScreenInfo *unique = NULL;

        for (n = 0, m = mons; m; m = m->next, n++)
            ;
        /* only consider unique geometries as separate screens */
        unique = ecalloc(nn, sizeof(XineramaScreenInfo));
        for (i = 0, j = 0; i < nn; i++)
            if (isuniquegeom(unique, j, &info[i]))
                memcpy(&unique[j++], &info[i], sizeof(XineramaScreenInfo));
        XFree(info);
        nn = j;

        /* new monitors if nn > n */
        for (i = n; i < nn; i++) {
            for (m = mons; m && m->next; m = m->next)
                ;
            if (m)
                m->next = createmon();
            else
                mons = createmon();
        }
        for (i = 0, m = mons; i < nn && m; m = m->next, i++)
            if (i >= n
                || unique[i].x_org != m->mx || unique[i].y_org != m->my
                || unique[i].width != m->mw || unique[i].height != m->mh) {
                dirty = 1;
                m->num = i;
                m->mx = m->wx = unique[i].x_org;
                m->my = m->wy = unique[i].y_org;
                m->mw = m->ww = unique[i].width;
                m->mh = m->wh = unique[i].height;
                updatebarpos(m);
            }
        /* removed monitors if n > nn */
        for (i = nn; i < n; i++) {
            for (m = mons; m && m->next; m = m->next)
                ;
            while ((c = m->clients)) {
                dirty = 1;
                m->clients = c->next;
                detachstack(c);
                c->mon = mons;
                switch (attachdirection) {
                    case 1:
                        attachabove(c);
                        break;
                    case 2:
                        attachaside(c);
                        break;
                    case 3:
                        attachbelow(c);
                        break;
                    case 4:
                        attachbottom(c);
                        break;
                    case 5:
                        attachtop(c);
                        break;
                    default:
                        attach(c);
                }
                attachstack(c);
            }
            if (m == selmon)
                selmon = mons;
            cleanupmon(m);
        }
        free(unique);
    } else
#endif /* XINERAMA */
    {  /* default monitor setup */
        if (!mons)
            mons = createmon();
        if (mons->mw != sw || mons->mh != sh) {
            dirty = 1;
            mons->mw = mons->ww = sw;
            mons->mh = mons->wh = sh;
            updatebarpos(mons);
        }
    }
    if (dirty) {
        selmon = mons;
        selmon = wintomon(root);
    }
    return dirty;
}

void
updatenumlockmask(void)
{
    unsigned int i, j;
    XModifierKeymap *modmap;

    numlockmask = 0;
    modmap = XGetModifierMapping(dpy);
    for (i = 0; i < 8; i++)
        for (j = 0; j < modmap->max_keypermod; j++)
            if (modmap->modifiermap[i * modmap->max_keypermod + j]
                == XKeysymToKeycode(dpy, XK_Num_Lock))
                numlockmask = (1 << i);
    XFreeModifiermap(modmap);
}

void
updatesizehints(Client *c)
{
    long msize;
    XSizeHints size;

    if (!XGetWMNormalHints(dpy, c->win, &size, &msize))
        /* size is uninitialized, ensure that size.flags aren't used */
        size.flags = PSize;
    if (size.flags & PBaseSize) {
        c->basew = size.base_width;
        c->baseh = size.base_height;
    } else if (size.flags & PMinSize) {
        c->basew = size.min_width;
        c->baseh = size.min_height;
    } else
        c->basew = c->baseh = 0;
    if (size.flags & PResizeInc) {
        c->incw = size.width_inc;
        c->inch = size.height_inc;
    } else
        c->incw = c->inch = 0;
    if (size.flags & PMaxSize) {
        c->maxw = size.max_width;
        c->maxh = size.max_height;
    } else
        c->maxw = c->maxh = 0;
    if (size.flags & PMinSize) {
        c->minw = size.min_width;
        c->minh = size.min_height;
    } else if (size.flags & PBaseSize) {
        c->minw = size.base_width;
        c->minh = size.base_height;
    } else
        c->minw = c->minh = 0;
    if (size.flags & PAspect) {
        c->mina = (float)size.min_aspect.y / size.min_aspect.x;
        c->maxa = (float)size.max_aspect.x / size.max_aspect.y;
    } else
        c->maxa = c->mina = 0.0;
    c->isfixed = (c->maxw && c->maxh && c->maxw == c->minw && c->maxh == c->minh);
    c->hintsvalid = 1;
}

void
updatestatus(void)
{
    if (!gettextprop(root, XA_WM_NAME, stext, sizeof(stext)))
        strcpy(stext, ""); // no shining of dwm version thru panel, when transparent
    drawbar(selmon);
}

void
updatetitle(Client *c)
{
    if (!gettextprop(c->win, netatom[NetWMName], c->name, sizeof c->name))
        gettextprop(c->win, XA_WM_NAME, c->name, sizeof c->name);
    if (c->name[0] == '\0') /* hack to mark broken clients */
        strcpy(c->name, broken);
}

void
updatewindowtype(Client *c)
{
    Atom state = getatomprop(c, netatom[NetWMState]);
    Atom wtype = getatomprop(c, netatom[NetWMWindowType]);

    if (state == netatom[NetWMFullscreen])
        setfullscreen(c, 1);
    if (wtype == netatom[NetWMWindowTypeDialog])
        c->isfloating = 1;
}

void
updatewmhints(Client *c)
{
    XWMHints *wmh;

    if ((wmh = XGetWMHints(dpy, c->win))) {
        if (c == selmon->sel && wmh->flags & XUrgencyHint) {
            wmh->flags &= ~XUrgencyHint;
            XSetWMHints(dpy, c->win, wmh);
        } else
            c->isurgent = (wmh->flags & XUrgencyHint) ? 1 : 0;
        if (wmh->flags & InputHint)
            c->neverfocus = !wmh->input;
        else
            c->neverfocus = 0;
        XFree(wmh);
    }
}

void
view(const Arg *arg)
{
    int i;
    unsigned int tmptag;

    if ((arg->ui & TAGMASK) == selmon->tagset[selmon->seltags])
        return;
    selmon->seltags ^= 1; /* toggle sel tagset */
    if (arg->ui & TAGMASK) {
        selmon->pertag->prevtag = selmon->pertag->curtag;
        selmon->tagset[selmon->seltags] = arg->ui & TAGMASK;
        if (arg->ui == ~0)
            selmon->pertag->curtag = 0;
        else {
            for (i = 0; !(arg->ui & 1 << i); i++)
                ;
            selmon->pertag->curtag = i + 1;
        }
    } else {
        tmptag = selmon->pertag->prevtag;
        selmon->pertag->prevtag = selmon->pertag->curtag;
        selmon->pertag->curtag = tmptag;
    }
    selmon->nmaster = selmon->pertag->nmasters[selmon->pertag->curtag];
    selmon->mfact = selmon->pertag->mfacts[selmon->pertag->curtag];
    selmon->sellt = selmon->pertag->sellts[selmon->pertag->curtag];
    selmon->lt[selmon->sellt] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt];
    selmon->lt[selmon->sellt ^ 1] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt ^ 1];
    focus(NULL);
    arrange(selmon);
}

void
viewall(const Arg *arg)
{
    Monitor *m;

    for (m = mons; m; m = m->next) {
        m->tagset[m->seltags] = arg->ui;
        arrange(m);
    }
    focus(NULL);
}

Client *
wintoclient(Window w)
{
    Client *c;
    Monitor *m;

    for (m = mons; m; m = m->next)
        for (c = m->clients; c; c = c->next)
            if (c->win == w)
                return c;
    return NULL;
}

Monitor *
wintomon(Window w)
{
    int x, y;
    Client *c;
    Monitor *m;

    if (w == root && getrootptr(&x, &y))
        return recttomon(x, y, 1, 1);
    for (m = mons; m; m = m->next)
        if (w == m->barwin)
            return m;
    if ((c = wintoclient(w)))
        return c->mon;
    return selmon;
}

void
warppointer(Monitor *selmon)
{
    if (!selmon->sel)
        return;
    if (!ispanel(selmon->sel) && !isnotifyd(selmon->sel) && selmon->sel->iswarppointer)
        XWarpPointer(dpy, None, selmon->sel->win, 0, 0, 0, 0, selmon->sel->w / 2, selmon->sel->h / 2);
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's). Other types of errors call Xlibs
 * default error handler, which may call exit. */
int
xerror(Display *dpy, XErrorEvent *ee)
{
    if (ee->error_code == BadWindow
        || (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch)
        || (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable)
        || (ee->request_code == X_PolyFillRectangle && ee->error_code == BadDrawable)
        || (ee->request_code == X_PolySegment && ee->error_code == BadDrawable)
        || (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch)
        || (ee->request_code == X_GrabButton && ee->error_code == BadAccess)
        || (ee->request_code == X_GrabKey && ee->error_code == BadAccess)
        || (ee->request_code == X_CopyArea && ee->error_code == BadDrawable))
        return 0;
    fprintf(stderr, "dwm: fatal error: request code=%d, error code=%d\n",
            ee->request_code, ee->error_code);
    return xerrorxlib(dpy, ee); /* may call exit */
}

int
xerrordummy(Display *dpy, XErrorEvent *ee)
{
    return 0;
}

/* Startup Error handler to check if another window manager
 * is already running. */
int
xerrorstart(Display *dpy, XErrorEvent *ee)
{
    die("dwm: another window manager is already running");
    return -1;
}

void
xinitvisual()
{
    XVisualInfo *infos;
    XRenderPictFormat *fmt;
    int nitems;
    int i;

    XVisualInfo tpl = {
        .screen = screen,
        .depth = 32,
        .class = TrueColor};
    long masks = VisualScreenMask | VisualDepthMask | VisualClassMask | VisualBitsPerRGBMask | VisualColormapSizeMask;

    infos = XGetVisualInfo(dpy, masks, &tpl, &nitems);
    visual = NULL;
    for (i = 0; i < nitems; i++) {
        fmt = XRenderFindVisualFormat(dpy, infos[i].visual);
        if (fmt->type == PictTypeDirect && fmt->direct.alphaMask) {
            visual = infos[i].visual;
            depth = infos[i].depth;
            cmap = XCreateColormap(dpy, root, visual, AllocNone);
            useargb = 1;
            break;
        }
    }

    XFree(infos);

    if (!visual) {
        visual = DefaultVisual(dpy, screen);
        depth = DefaultDepth(dpy, screen);
        cmap = DefaultColormap(dpy, screen);
    }
}

void
zoom(const Arg *arg)
{
    Client *c = selmon->sel;

    if (!selmon->lt[selmon->sellt]->arrange || !c || c->isfloating)
        return;
    if (c == nexttiled(selmon->clients) && !(c = nexttiled(c->next)))
        return;
    pop(c);
}

int
main(int argc, char *argv[])
{
    if (argc == 2 && !strcmp("-v", argv[1]))
        die("dwm-" VERSION);
    else if (argc != 1)
        die("usage: dwm [-v]");
    if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
        fputs("warning: no locale support\n", stderr);
    if (!(dpy = XOpenDisplay(NULL)))
        die("dwm: cannot open display");
    checkotherwm();
    setup();
#ifdef __OpenBSD__
    if (pledge("stdio rpath proc exec", NULL) == -1)
        die("pledge");
#endif /* __OpenBSD__ */
    scan();
    runautostart();
    run();
    cleanup();
    XCloseDisplay(dpy);
    return EXIT_SUCCESS;
}
