/* See LICENSE file for copyright and license details. */
#define SESSION_FILE "/tmp/dwm-session"
#define ICONSIZE 16   /* icon size */
#define ICONSPACING 8 /* space between icon and title */

/* appearance */
static const unsigned int borderpx = 2;        /* border pixel of windows */
static const unsigned int gappx    = 8;        /* gap pixel between windows */
static const unsigned int snap     = 32;       /* snap pixel */
static const int rmaster           = 0;        /* 1 means master-area is initially on the right */
static const char panel[][32]      = { "xfce4-panel", "KMagnifier", "Clock", "Calculator" }; /* name & cls of panel win */
static const int entagmon          = 1;		/* 0 means dosn't send selclient when switch monitor*/
static const int viewontag         = 1;        /* Switch view on tag switch */
static const int showbar           = 1;        /* 0 means no bar */
static const int topbar            = 1;        /* 0 means bottom bar */
static const int vertpad           = 0;        /* vertical padding of bar */
static const int sidepad           = 0;        /* horizontal padding of bar */
static const int user_bh           = 32;       /* 0 means that dwm will calculate bar height, >= 1 means dwm will user_bh as bar height */
static const int min_tag           = 4;        /* min number of tag */
static const char *fonts[] = { "VictorMono Nerd Font:style=Bold:size=16:antialias=true:autohint=true",
								"Symbola:size=16:antialias=true:autohint=true",
								"LXGW WenKai:style=Medium:size=16:antialias=true:autohint=true",
								"Comic Code:style=Medium:size=16:antialias=true:autohint=true", };
static const unsigned int baralpha    = 0xd0;
static const unsigned int borderalpha = OPAQUE;
static const char fgcolor1[] = "#0e0e0e";
static const char bgcolor1[] = "#efefef";
static const char bdcolor1[] = "#efefef";

static const char fgcolor2[] = "#0e0e0e";
static const char bgcolor2[] = "#b8bb26";
static const char bdcolor2[] = "#b8bb26";

static const char fgcolor3[] = "#0e0e0e";
static const char bgcolor3[] = "#b8bb26";
static const char bdcolor3[] = "#b8bb26";

static const char fgcolor4[] = "#efefef";
static const char bgcolor4[] = "#0e0e0e";
static const char bdcolor4[] = "#0e0e0e";

static const char fgcolor5[] = "#b8bb26";
static const char bgcolor5[] = "#0e0e0e";
static const char bdcolor5[] = "#0e0e0e";

static const char fgcolor6[] = "#0e0e0e";
static const char bgcolor6[] = "#b8bb26";
static const char bdcolor6[] = "#b8bb26";

static const char fgcolor7[] = "#efefef";
static const char bgcolor7[] = "#0e0e0e";
static const char bdcolor7[] = "#0e0e0e";

static const char fgcolor8[] = "#0e0e0e";
static const char bgcolor8[] = "#b8bb26";
static const char bdcolor8[] = "#b8bb26";

static const char *colors[][3] = {
	/*                         fg         bg         border   */
	[SchemeNorm]        =    { fgcolor1, bgcolor1, bdcolor1 },
	[SchemeSel]         =    { fgcolor2, bgcolor2, bdcolor2 },
	[SchemeStatus]      =    { fgcolor3, bgcolor3, bdcolor3 },
	[SchemeTagsSel]     =    { fgcolor4, bgcolor4, bdcolor4 },
	[SchemeTagsHover]   =    { fgcolor5, bgcolor5, bdcolor5 },
	[SchemeTagsNorm]    =    { fgcolor6, bgcolor6, bdcolor6 },
	[SchemeInfoSel]     =    { fgcolor7, bgcolor7, bdcolor7 },
	[SchemeInfoNorm]    =    { fgcolor8, bgcolor8, bdcolor8 },
};

static const unsigned int alphas[][3] = {
    /*               fg      bg        border*/
    [SchemeNorm] = { OPAQUE, baralpha, borderalpha },
	[SchemeSel]  = { OPAQUE, baralpha, borderalpha },
    [SchemeStatus] = { OPAQUE, baralpha, borderalpha },
    [SchemeTagsSel]  = { OPAQUE, baralpha, borderalpha },
    [SchemeTagsHover]  = { OPAQUE, baralpha, borderalpha },
    [SchemeTagsNorm] = { OPAQUE, baralpha, borderalpha },
    [SchemeInfoSel]  = { OPAQUE, baralpha, borderalpha },
    [SchemeInfoNorm] = { OPAQUE, baralpha, borderalpha },
};

/* Select the font index for you statusbar
 * the index is zero based*/
static const int statusfontindex = 2;
/* tagging */
static const char *tags[] = { "", "󰰶", "󰰡", "󰰛", "󰰰", "󰰰", "󰰭", "󰰘", "󰎣" };

/* Lockfile */
static char lockfile[] = "/tmp/dwm.lock";

/* 1 means dynamically changing isfloating rule of a selected client, and vice versa. */
static const int dynamicrule = 0;
static char regexarray[][255] = { "^nvim$", "^(Minecraft Launcher|Sign in to Minecraft|Welcome to Xbox)$" };
static Rule rules[] = {
	/* xprop(1):
	 *	WM_CLASS(STRING) = instance, class
	 *	WM_NAME(STRING) = title
	 */
	/* class                  instance      title         tags mask    viewontag  isfloating  iscentered forcetile  monitor  isfactor  factor x,y,w,h(double)  borderpx  iswarppointer */
 // { "st",                   NULL,         NULL,         0,           1,         1,          1,         1,         -1,      1,        1.0,1.0,1.0,1.0,        2,        1 },
    { "st",                   NULL,         NULL,         0,           0,         1,          0,         1,         -1,      1,        0.9,0.9,0.9,0.9,        2,        1 },
    { "kitty",                NULL,         NULL,         0,           0,         1,          0,         1,         -1,      1,        0.9,0.9,0.9,0.9,        2,        1 },
    { "okular",               NULL,         NULL,         1 << 1,      1,         0,          0,         1,         -1,      0,        0.9,0.9,0.9,0.9,        2,        1 },
    { "firefox-esr",          NULL,         NULL,         1 << 2,      1,         1,          0,         1,         -1,      1,        0.9,0.9,0.9,0.9,        2,        1 },
    { "Chromium",             NULL,         NULL,         0,           0,         1,          0,         1,         -1,      1,        0.9,0.9,0.9,0.9,        2,        1 },
    { "code-oss",             NULL,         NULL,         1 << 1,      1,         1,          0,         0,         -1,      1,        0.9,0.9,0.9,0.9,        2,        1 },
    { "gnome-calculator",     NULL,         NULL,         (1 << 9) - 1,0,         1,          0,         0,         -1,      1,        0.23,1.0,1.0,0.32,      2,        0 },
    { "rnote",                NULL,         NULL,         0,           0,         1,          0,         0,         -1,      1,        0.9,0.9,0.9,0.9,        2,        1 },
    { "kclock",               NULL,         NULL,         (1 << 9) - 1,0,         1,          0,         0,         -1,      1,        0.23,1.0,1.0,0.70,      2,        0 },
    { "GoldenDict",           NULL,         NULL,         0,           0,         1,          0,         0,         -1,      1,        0.5,0.5,1.0,1.0,        2,        1 },
    { "Clash for Windows",    NULL,         NULL,         0,           0,         1,          1,         0,         -1,      0,        0.9,0.9,0.9,0.9,        2,        1 },
    { "Vmware",               NULL,         NULL,         1 << 7,      1,         1,          0,         1,         -1,      1,        0.9,0.9,0.9,0.9,        2,        1 },
    { "VirtualBox Manager",   NULL,         NULL,         1 << 8,      1,         1,          0,         1,         -1,      1,        0.9,0.9,0.9,0.9,        2,        0 },
    { "VirtualBox Machine",   NULL,         NULL,         0,           0,         1,          0,         0,         -1,      1,        0.9,0.9,0.9,0.9,        2,        0 },
    { "steam",                NULL,         NULL,         1 << 3,      1,         1,          0,         0,         -1,      1,        0.9,0.9,0.9,0.9,        2,        0 },
    { "Xfce4-notifyd",        NULL,         NULL,         0,           0,         1,          0,         0,         -1,      0,        0.23,1.0,1.0,1.0,       0,        0 },
    { NULL,                   NULL,         regexarray[1],1 << 3,      1,         1,          0,         0,         -1,      1,        0.9,0.9,0.9,0.9,        2,        1 },
    { NULL,                   NULL,         regexarray[0],0,           0,         1,          0,         0,         -1,      1,        0.6,1.0,1.0,0.8,        2,        1 },
    { "kmag",                 NULL,         NULL,         (1 << 9) - 1,0,         1,          0,         0,         -1,      1,        0.4,1.0,1.0,0.2,        0,        0 },
    { "Xfce4-panel",          NULL,         NULL,         (1 << 9) - 1,0,         1,          1,         0,         -1,      0,        1.0,1.0,1.0,1.0,        0,        0 },
    { broken,                 NULL,         NULL,         0,           0,         1,          1,         0,         -1,      1,        0.9,0.9,0.9,0.9,        2,        1 },
    { NULL,                   NULL,         NULL,         0,           0,         1,          1,         0,         -1,      1,        0.9,0.9,0.9,0.9,        2,        0 },
};

/* layout(s) */
static const float mfact         = 0.63; /* factor of master area size [0.05..0.95] */
static const int nmaster         = 1;    /* number of clients in master area */
static const int resizehints     = 1;    /* 1 means respect size hints in tiled resizals */
static const int lockfullscreen  = 0; /* 1 will force focus on the fullscreen window */
static const int attachdirection = 4;    /* 0 default, 1 above, 2 aside, 3 below, 4 bottom, 5 top */

static const Layout layouts[] = {
    /* first entry is default */
	/* symbol     arrange function */
	{ "󰬛",        tile },
	{ "󰬔",        monocle },
	{ "󰬚",        spiral },
 	{ "󰬋",        dwindle },
	{ NULL,       NULL },
};

/* key definitions */
#define MODKEY Mod1Mask
#define SUPERKEY Mod4Mask
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
	{ MODKEY|SUPERKEY,              KEY,      viewall,        {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
	{ MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
static const char *menucmd[]  = { "/bin/sh", "-c" , "~/.dwm/scripts/rofi/powermenu.sh" , NULL };
static const char *roficmd[]  = { "/bin/sh", "-c" , "~/.dwm/scripts/rofi/launcher.sh" , NULL };
static const char *scrkcmd[]  = { "/bin/sh", "-c" , "~/.dwm/scripts/screenkey/screenkey.sh" , NULL };
static const char *kmagcmd[]  = { "/bin/sh", "-c" , "~/.dwm/scripts/kmagnifier/kmagnifier.sh" , NULL };
static const char *rangcmd[]  = { "kitty", "ranger", NULL };
static const char *flamcmd[]  = { "flameshot", "gui", NULL };
static const char *thuncmd[]  = { "thunar", NULL };
static const char *termcmd[]  = { "kitty", NULL };

static const Key keys[] = {
	/* modifier                     key            function            argument */
	{ MODKEY,                       XK_p,          spawn,              {.v = roficmd } },
	{ MODKEY|ShiftMask,             XK_p,          spawn,              {.v = menucmd } },
	{ MODKEY,    			        XK_e,          spawn,              {.v = rangcmd } },
	{ MODKEY|ShiftMask,            	XK_e,          spawn,              {.v = thuncmd } },
	{ MODKEY,                       XK_s,          spawn,              {.v = flamcmd } },
	{ MODKEY|ShiftMask,             XK_s,          spawn,              {.v = scrkcmd } },
	{ MODKEY|ShiftMask,             XK_z,          spawn,              {.v = kmagcmd } },
	{ MODKEY|ShiftMask,             XK_Return,     spawn,              {.v = termcmd } },
	{ MODKEY,                       XK_b,          togglebar,          {0} },
	{ MODKEY|ShiftMask,             XK_h,          rotatestack,        {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_l,          rotatestack,        {.i = -1 } },
	{ MODKEY,                       XK_j,          focusstackvis,      {.i = +1 } },
	{ MODKEY,                       XK_k,          focusstackvis,      {.i = -1 } },
	{ MODKEY,                       XK_i,          incnmaster,         {.i = +1 } },
	{ MODKEY,                       XK_d,          incnmaster,         {.i = -1 } },
	{ MODKEY,                       XK_h,          setmfact,           {.f = -0.05} },
	{ MODKEY,                       XK_l,          setmfact,           {.f = +0.05} },
	{ MODKEY|ShiftMask,             XK_j,          movestack,          {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_k,          movestack,          {.i = -1 } },
	{ MODKEY,                       XK_Return,     zoom,               {0} },
	{ MODKEY,                       XK_Tab,        view,               {0} },
	{ MODKEY|ShiftMask,             XK_c,          killclient,         {0} },
	{ MODKEY,                       XK_space,      cyclelayout,        {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_space,      togglefloating,     {0} },
	{ MODKEY,                       XK_z,          togglelayer,        {0} },
	{ MODKEY,                       XK_r,          togglermaster,      {0} },
	{ MODKEY,                       XK_slash,      show,               {0} },
	{ MODKEY|ShiftMask,             XK_slash,      showall,            {0} },
	{ MODKEY,             			XK_semicolon,  hide,               {0} },
	{ MODKEY|ShiftMask,             XK_semicolon,  hideall,            {0} },
	{ MODKEY,             			XK_m,          resetnmaster,       {0} },
	{ MODKEY,                       XK_0,          view,               {.ui = ~0 } },
	{ MODKEY|ShiftMask,             XK_0,          tag,                {.ui = ~0 } },
	{ MODKEY,                       XK_apostrophe, focusstackhid,      {.i = -1 } },
	{ MODKEY|ShiftMask,             XK_apostrophe, focusstackhid,      {.i = +1 } },
	{ MODKEY,                       XK_comma,      focusmon,           {.i = -1 } },
	{ MODKEY,                       XK_period,     focusmon,           {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_comma,      tagmon,             {.i = -1 } },
	{ MODKEY|ShiftMask,             XK_period,     tagmon,             {.i = +1 } },
	{ MODKEY,                       XK_minus,      setgaps,            {.i = -1 } },
	{ MODKEY,                       XK_equal,      setgaps,            {.i = +1 } },
    { MODKEY|ShiftMask,             XK_equal,      setgaps,            {.i = 0  } },
    { MODKEY|ShiftMask,             XK_Up,         movethrow,          {.ui = WIN_N  }},
    { MODKEY|ShiftMask,             XK_Down,       movethrow,          {.ui = WIN_S  }},
    { MODKEY|ShiftMask,             XK_Left,       movethrow,          {.ui = WIN_W  }},
    { MODKEY|ShiftMask,             XK_Right,      movethrow,          {.ui = WIN_E  }},
    { MODKEY|ShiftMask,             XK_m,          movethrow,          {.ui = WIN_C  }},
	TAGKEYS(                        XK_1,                              0)
	TAGKEYS(                        XK_2,                              1)
	TAGKEYS(                        XK_3,                              2)
	TAGKEYS(                        XK_4,                              3)
	TAGKEYS(                        XK_5,                              4)
	TAGKEYS(                        XK_6,                              5)
	TAGKEYS(                        XK_7,                              6)
	TAGKEYS(                        XK_8,                              7)
	TAGKEYS(                        XK_9,                              8)
	{ MODKEY|ShiftMask,             XK_q,          quit,               {0} },
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static const Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkLtSymbol,          0,              Button1,        cyclelayout,      {.i = +1 } },
	{ ClkLtSymbol,          0,              Button3,        cyclelayout,      {.i = -1 } },
	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
	{ ClkWinTitle,          0,              Button2,        zoom,           {0} },
	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         ControlMask,    Button1,        raiseclient,    {0} },
	{ ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
	{ ClkTagBar,            0,              Button1,        view,           {0} },
	{ ClkTagBar,            0,              Button3,        toggleview,     {0} },
	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
};
