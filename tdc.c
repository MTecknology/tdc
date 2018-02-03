/**
 * This file is part of Tiny Dockable Clock (TDC).
 *
 * TDC is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * TDC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with TDC.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright: (C) Michael Lustfield <michael@lustfield.net>
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include <X11/Xresource.h>
#include <X11/StringDefs.h>
#include <time.h>
#include <signal.h>

#include <stdio.h>
double calfontsize = 14.0;
char *calfontname = "Mono";
char *hlcolorname = "grey30";
char curr_cal[8][30];
#define NUM_OPTS 23

int cal = 0;
int running = 0;
int windowwidth = 64;
int fontweight = 100;
double fontsize = 12.0;
char *fontname = "Mono";
char *timeformat = "%T";
char *colorname = "white";

static XrmOptionDescRec opt_table[] = {
  {"--font=",        "*font",        XrmoptionStickyArg, (XPointer)NULL},
  {"--bold",         "*bold",        XrmoptionNoArg,     (XPointer)""},
  {"--fontsize=",    "*fontsize",    XrmoptionStickyArg, (XPointer)NULL},
  {"--color=",       "*color",       XrmoptionStickyArg, (XPointer)NULL},
  {"--width=",       "*width",       XrmoptionStickyArg, (XPointer)NULL},
  {"--format=",      "*format",      XrmoptionStickyArg, (XPointer)NULL},
  {"--calfont=",     "*calfont",     XrmoptionStickyArg, (XPointer)NULL},
  {"--calfontsize=", "*calfontsize", XrmoptionStickyArg, (XPointer)NULL},
  {"--hlcolor=",     "*hlcolor",     XrmoptionStickyArg, (XPointer)NULL},
  {"--help",         "*help",        XrmoptionNoArg,     (XPointer)""},
  {"--version",      "*version",     XrmoptionNoArg,     (XPointer)""},
  {"--enable-cal",   "*enable-cal",  XrmoptionNoArg,     (XPointer)""},
  {"-t",             "*font",        XrmoptionSepArg,    (XPointer)NULL},
  {"-b",             "*bold",        XrmoptionNoArg,     (XPointer)""},
  {"-s",             "*fontsize",    XrmoptionSepArg,    (XPointer)NULL},
  {"-c",             "*color",       XrmoptionSepArg,    (XPointer)NULL},
  {"-w",             "*width",       XrmoptionSepArg,    (XPointer)NULL},
  {"-f",             "*format",      XrmoptionSepArg,    (XPointer)NULL},
  {"-h",             "*help",        XrmoptionNoArg,     (XPointer)""},
  {"-v",             "*version",     XrmoptionNoArg,     (XPointer)""},
};

static void gettime(char *strtime) {
  time_t curtime;
  struct tm *curtime_tm;

  curtime = time(NULL);
  curtime_tm = localtime(&curtime);
  strftime(strtime, 49, timeformat, curtime_tm);
}

void handle_term() {
  running = 0;
}

void die(const char* str) {
  fputs(str, stderr);
  fflush(stderr);
  exit(1);
}

void get_params(Display *display, int argc, char *argv[]) {
  XrmDatabase database;
  int nargc = argc;
  char **nargv = argv;
  char *type;
  XrmValue xrmval;
  char *xdefaults;

  XrmInitialize();
  database = XrmGetDatabase(display);
  
  /* merge in Xdefaults */
  xdefaults = malloc(strlen(getenv("HOME")) + strlen("/.Xdefaults") + 1);
  if (!xdefaults) {
    die("Memory allocation failed");
  }
  snprintf(xdefaults, sizeof(xdefaults), "%s/.Xdefaults", getenv("HOME"));
  XrmCombineFileDatabase(xdefaults, &database, True);
  free(xdefaults);

  /* parse command line */
  XrmParseCommand(&database, opt_table, NUM_OPTS, "tdc", &nargc, nargv);

  /* check for help option */
  if (XrmGetResource(database, "tdc.help", "tdc.help", &type, &xrmval) == True) {
    printf("Usage: tdc [options]\n\n");
    printf("Options:\n");
    printf("-v,  --version:     Print version information\n");
    printf("-h,  --help:        This text\n");
    printf("-t,  --font:        The font face to use.  Default is Mono.\n");
    printf("-b,  --bold:        Use a bold font.\n");
    printf("-s,  --fontsize:    The font size to use.  Default is 12.0.\n");
    printf("-c,  --color:       The font color to use.  Default is white.\n");
    printf("-w,  --width:       The width of the dockapp window.  Default is 64.\n");
    printf("-f,  --format:      The time format to use.  Default is %%T.  You can\n");
    printf("                    get format codes from the man page for strftime.\n");
    printf("     --enable-cal:  Enable the calendar feature which is opened by clicking the clock.\n");
    printf("     --calfont:     The font that will be used in the calendar.  Default is Mono.\n");
    printf("     --calfontsize: The font size used in the calendar.  Default is 14.0.\n");
    printf("     --hlcolor:     The highlight color to use in the calendar.  Default is grey30.\n");
    printf("You can also put these in your ~/.Xdefaults file:\n");
    printf("tdc*font\n");
    printf("tdc*fontsize\n");
    printf("tdc*enable-cal\n");
    printf("tdc*calfont\n");
    printf("tdc*calfontsize\n");
    printf("tdc*color\n");
    printf("tdc*hlcolor\n");
    printf("tdc*width\n");
    printf("tdc*format\n\n");
    exit(0);
  }

  /* check for version request */
  if (XrmGetResource(database, "tdc.version", "tdc.version", &type, &xrmval) == True) {
    printf("tdc 1.6\n");
    exit(0);
  }

  /* get options out of it */
  if (XrmGetResource(database, "tdc.enable-cal", "tdc.enable-cal", &type, &xrmval) == True) {
    cal = 1;
  }
  
  if (XrmGetResource(database, "tdc.font", "tdc.font", &type, &xrmval) == True) {
    if (strcmp(type, "String")) {
      die("Option \"font\" was in an unexpected format!");
    }
    fontname = xrmval.addr;
  }

  if (XrmGetResource(database, "tdc.bold", "tdc.bold", &type, &xrmval) == True) {
    fontweight = 200; /* 200 means bold */
  }

  if (XrmGetResource(database, "tdc.fontsize", "tdc.fontsize", &type, &xrmval) == True) {
    if (strcmp(type, "String")) {
      die("Option \"fontsize\" was in an unexpected format!");
    }
    fontsize = strtod(xrmval.addr, NULL);
  }

  if (XrmGetResource(database, "tdc.color", "tdc.color", &type, &xrmval) == True) {
    if (strcmp(type, "String")) {
      die("Option \"color\" was in an unexpected format!");
    }
    colorname = xrmval.addr;
  }

  if (XrmGetResource(database, "tdc.width", "tdc.width", &type, &xrmval) == True) {
    if (strcmp(type, "String")) {
      die("Option \"width\" was in an unexpected format!");
    }
    windowwidth = strtod(xrmval.addr, NULL);
  }

  if (XrmGetResource(database, "tdc.format", "tdc.format", &type, &xrmval) == True) {
    if (strcmp(type, "String")) {
      die("Option \"format\" was in an unexpected format!");
    }
    timeformat = xrmval.addr;
  }  

  if (XrmGetResource(database, "tdc.calfont", "tdc.calfont", &type, &xrmval) == True) {
    if (strcmp(type, "String")) {
      die("Option \"calfont\" was in an unexpected format!");
    }
    calfontname = xrmval.addr;
  }

  if (XrmGetResource(database, "tdc.calfontsize", "tdc.calfontsize", &type, &xrmval) == True) {
    if (strcmp(type, "String")) {
      die("Option \"calfontsize\" was in an unexpected format!");
    }
    calfontsize = strtod(xrmval.addr, NULL);
  }

  if (XrmGetResource(database, "tdc.hlcolor", "tdc.hlcolor", &type, &xrmval) == True) {
    if (strcmp(type, "String")) {
      die("Option \"hlcolor\" was in an unexpected format!");
    }
    hlcolorname = xrmval.addr;
  }
}

/*Calculates where to place calendar window*/
void locateCalendar(Display *display, int screen, Window dockapp, Window root, Window calendar) {
  Window tmp;
  XWindowAttributes attr;
  int d_x, d_y, d_w, d_h;
  int cal_x = 0, cal_y = 0, cal_w, cal_h;
  int res_w, res_h;

  res_w = XDisplayWidth(display, screen);
  res_h = XDisplayHeight(display, screen);
  XTranslateCoordinates(display, dockapp, root, 0, 0, &d_x, &d_y, &tmp);
  XGetWindowAttributes(display, dockapp, &attr);
  d_w = attr.width;
  d_h = attr.height;
  XGetWindowAttributes(display, calendar, &attr);
  cal_w = attr.width;
  cal_h = attr.height;

  if ((d_x < res_w / 2) && (d_y < res_h / 2)) {
    /*Top left */
    cal_x = d_x;
    cal_y = (d_y + d_h);
  }
  else if ((d_x >= res_w / 2) && (d_y < res_h / 2)) {
    /*Top right */
    cal_x = (d_x + d_w) - cal_w;
    cal_y = (d_y + d_h);
  }
  else if ((d_x < res_w / 2) && (d_y >= res_h / 2)) {
    /*Bottom left */
    cal_x = d_x;
    cal_y = d_y - cal_h;
  }
  else if ((d_x >= res_w / 2) && (d_y >= res_h / 2)) {
    /*Bottom right */
    cal_x = (d_x + d_w) - cal_w;
    cal_y = d_y - cal_h;
  }

  XMoveWindow(display, calendar, cal_x, cal_y);
}

void shiftdate(int *month, int *year, int direction) {
  *month += direction;
  if (*month == 0) {
    *month = 12;
    --*year;
  }
  else if (*month == 13) {
    *month = 1;
    ++*year;
  }
}

int getday() {
  time_t curtime;
  struct tm *curtime_tm;

  curtime = time(NULL);
  curtime_tm = localtime(&curtime);
  return curtime_tm->tm_mday;
}

int getmonth() {
  time_t curtime;
  struct tm *curtime_tm;

  curtime = time(NULL);
  curtime_tm = localtime(&curtime);
  return curtime_tm->tm_mon + 1;
}

int getyear() {
  time_t curtime;
  struct tm *curtime_tm;

  curtime = time(NULL);
  curtime_tm = localtime(&curtime);
  return curtime_tm->tm_year + 1900;
}

void get_calendar(char line[8][30], int month, int year) {
  FILE *fp;
  int i = 0;
  char command[30];
  char buff[8][30];

  snprintf(command, sizeof(command), "cal -h %d %d", month, year);

  fp = popen(command, "r");
  while (fgets(buff[i], sizeof buff[i], fp)) {
    i++;
  }
  pclose(fp);

  for (i = 0; i < 8; i++) {
    snprintf(line[i], sizeof(line[i]), "%-30s", buff[i]);
  }
}

void findtoday(int *todayi, int *todayj, int cal_m, int cal_y) {
  char needle[3];
  char *pos_n, *pos_h;

  *todayi = -1;
  *todayj = -1;

  if (cal_m == getmonth() && cal_y == getyear()) {
    snprintf(needle, sizeof(needle), "%d", getday());

    pos_h = (char*) &curr_cal[2];
    pos_n = (char*) strstr(curr_cal[2], needle);

    *todayi = (pos_n - pos_h) / sizeof curr_cal[2] + 2;
    *todayj = (pos_n - pos_h) % sizeof curr_cal[2];

    if (getday() < 10) {
      -- * todayj;
    }
  }
}

void paintCalendar(Display *display, int cal_m, int cal_y, XftDraw *caldraw,
                   XftFont *font, XftColor *xftcolor, XftColor *xfthlcolor) {
  int i = 0, j = 0;
  int p_x = 0, p_y = 0;
  int todayi = 0, todayj = 0;
  char leftarrow[] = "<";
  char rightarrow[] = ">";
  char buff[2];
  XGlyphInfo extents;
  int glyph_w, glyph_h;

  XftTextExtentsUtf8(display, font, (FcChar8*) "0", strlen("0"), &extents);
  glyph_w = extents.xOff;
  glyph_h = extents.height * 1.8;

  get_calendar(curr_cal, cal_m, cal_y);
  findtoday(&todayi, &todayj, cal_m, cal_y);

  for (i = 0; i < 8; i++) {
    for (j = 0; j < 20; j++) {
      p_x = j * glyph_w + 1;
      p_y = (i + 1) * glyph_h;
      snprintf(buff, sizeof(buff), "%c", curr_cal[i][j]);
      if ((int) buff[0] != 10) {
        /*make sure not to print newline chars */
        if (i == todayi && (j == todayj || j == todayj + 1)) {
          XftDrawRect(caldraw, xfthlcolor, p_x - extents.x, p_y - extents.y, extents.xOff, extents.height);
          XftDrawStringUtf8(caldraw, xftcolor, font, p_x, p_y, (FcChar8*) buff, strlen(buff));
        }
        else {
          XftDrawStringUtf8(caldraw, xftcolor, font, p_x, p_y, (FcChar8*) buff, strlen(buff));
        }
      }
    }
  }

  XftDrawStringUtf8(caldraw, xftcolor, font, 1, glyph_h, (FcChar8*) leftarrow, strlen(leftarrow));
  XftDrawStringUtf8(caldraw, xftcolor, font, glyph_w *19, glyph_h, (FcChar8*) rightarrow, strlen(rightarrow));

  XFlush(display);
}

int main(int argc, char *argv[]) {
  /* X stuff */
  Display *display;
  Window dockapp;
  XWMHints wm_hints;
  XftFont *font;
  XftDraw *draw;
  XftColor xftcolor;
  XRenderColor color;
  XColor c;
  int screen;
  Colormap colormap;
  Window root;
  Visual *vis;
  XGlyphInfo extents;
  XEvent event;
  int x, y;
  Window calendar = (Window)NULL;
  XSetWindowAttributes attributes;
  XftFont *calfont = NULL;
  XftDraw *caldraw = NULL;
  XftColor xfthlcolor;
  int cal_m = 0, cal_y = 0;
  int m_x = 0;
  int glyph_w = 0, glyph_h = 0;
  int calopen = 0;
  char *appname = "tdc";

  /* Time stuff */
  int xfd, selectret = 0;
  fd_set fds;
  struct timeval timeout;
  char strtime[50];

  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
  struct sigaction act = {{0}};
  #pragma GCC diagnostic pop

  act.sa_handler = &handle_term;
  sigaction(SIGTERM, &act, NULL);
  sigaction(SIGINT, &act, NULL);
  running = 1;

  display = XOpenDisplay(NULL);
  if (!display) {
    fprintf(stderr, "Couldn't open X\n");
    fflush(stderr);
    exit(1);
  }

  /* command line/xresources: sets fonts etc */
  get_params(display, argc, argv); 

  xfd = ConnectionNumber(display);
  if (xfd == -1) {
    fprintf(stderr, "Couldn't get X filedescriptor\n");
    fflush(stderr);
    exit(1);
  }

  screen = DefaultScreen(display);
  colormap = DefaultColormap(display, screen);
  root = DefaultRootWindow(display);
  vis = DefaultVisual(display, screen);
  dockapp = XCreateSimpleWindow(display, root, 0, 0, windowwidth, 24, 0, 0, 0);

  wm_hints.initial_state = WithdrawnState;
  wm_hints.icon_window = wm_hints.window_group = dockapp;
  wm_hints.flags = StateHint | IconWindowHint;
  XSetWMHints(display, dockapp, &wm_hints);
  XSetCommand(display, dockapp, argv, argc);

  XSetWindowBackgroundPixmap(display, dockapp, ParentRelative);

  XStoreName(display, dockapp, appname);

  if (cal) {
    cal_m = getmonth();
    cal_y = getyear();
  }

  gettime(strtime);
  font = XftFontOpen(display, screen, XFT_FAMILY, XftTypeString, fontname, XFT_WEIGHT, XftTypeInteger,
                     fontweight, XFT_PIXEL_SIZE, XftTypeDouble, fontsize, NULL);
  if (cal) {
    calfont = XftFontOpen(display, screen, XFT_FAMILY, XftTypeString, calfontname, XFT_PIXEL_SIZE,
                          XftTypeDouble, calfontsize, NULL);
    XftTextExtentsUtf8(display, calfont, (FcChar8*) "0", strlen("0"), &extents);
    glyph_w = extents.xOff;
    glyph_h = extents.height * 1.8;
  }

  XftTextExtents8(display, font, (unsigned char*)strtime, strlen(strtime), &extents);
  x = (windowwidth-extents.width)/2 + extents.x;
  y = (24-extents.height)/2 + extents.y;
  if (x < 0 || y < 0) {
    fprintf(stderr, "Uh oh, text doesn't fit inside window\n");
    fflush(stderr);
  }
  draw = XftDrawCreate(display, dockapp, vis, colormap);
  if (XParseColor(display, colormap, colorname, &c) == None) {
    fprintf(stderr, "Couldn't parse color '%s'\n", colorname);
    fflush(stderr);
    color.red = 0xffff;
    color.green = 0xffff;
    color.blue = 0xffff;
  }
  else {
    color.red = c.red;
    color.blue = c.blue;
    color.green = c.green;
  }
  color.alpha = 0xffff;
  XftColorAllocValue(display, vis, colormap, &color, &xftcolor);

  if (cal) {
    if (XParseColor(display, colormap, hlcolorname, &c) == None) {
      fprintf(stderr, "Couldn't parse color '%s'\n", hlcolorname);
      fflush(stderr);
      color.red = 0xffff;
      color.green = 0xffff;
      color.blue = 0xffff;
    }
    else {
      color.red = c.red;
      color.blue = c.blue;
      color.green = c.green;
    }

    color.alpha = 0xffff;
    XftColorAllocValue(display, vis, colormap, &color, &xfthlcolor);

    XSelectInput(display, dockapp, ExposureMask | StructureNotifyMask | ButtonPressMask | ButtonReleaseMask);
    XMapWindow(display, dockapp);
    XFlush(display);

    /*Set up calendar window */
    int loc_x = 0;
    int loc_y = 0;

    calendar = XCreateSimpleWindow(display, root, loc_x, loc_y, glyph_w * 20 + 2,
                            glyph_h * 8 + (glyph_h - glyph_h / 1.8) +
                            1, 1, BlackPixel(display, DefaultScreen(display)),
                            WhitePixel(display, DefaultScreen(display)));
    attributes.override_redirect = True;
    XChangeWindowAttributes(display, calendar, CWOverrideRedirect, &attributes);
    XSelectInput(display, calendar, ExposureMask | ButtonPressMask);
    caldraw = XftDrawCreate(display, calendar, vis, colormap);
  }
  else {
    XSelectInput(display, dockapp, ExposureMask);
    XMapWindow(display, dockapp);
    XFlush(display);
  }

  timeout.tv_sec = timeout.tv_usec = 0;

  while (running) {
    FD_ZERO(&fds);
    FD_SET(xfd, &fds);

    selectret = select(xfd+1, &fds, NULL, NULL, &timeout);

    switch (selectret) {
      case -1:
        continue;
      case 0:
        gettime(strtime);
        XClearWindow(display, dockapp);
        XftDrawString8(draw, &xftcolor, font, x, y, (unsigned char*)strtime, strlen(strtime));
        XFlush(display);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
	break;
      case 1:
        if (FD_ISSET(xfd, &fds)) {
          XNextEvent(display, &event);
          if (cal && event.type == ButtonPress) {
            if (event.xbutton.window == dockapp && event.xbutton.button == 1) {
              if (calopen) {
                XUnmapWindow(display, calendar);
                calopen = 0;
              }
              else {
                cal_m = getmonth();
                cal_y = getyear();
                get_calendar(curr_cal, cal_m, cal_y);
                XSetWindowBackground(display, calendar, XGetPixel(XGetImage(
                        display, dockapp, 0, 0, 1, 1, AllPlanes,
                        XYPixmap), 0, 0));
                locateCalendar(display, screen, dockapp, root, calendar);
                XMapWindow(display, calendar);
                calopen = 1;
              }
            }
            else if (event.xbutton.button == 1) {
              m_x = event.xbutton.x;
              if (m_x >= 2 && m_x < (glyph_w * 20 + 2) / 2) {
                shiftdate(&cal_m, &cal_y, -1);
                XClearWindow(display, calendar);
                paintCalendar(display, cal_m, cal_y, caldraw, calfont, &xftcolor, &xfthlcolor);
            }
            else if (m_x >= (glyph_w * 20 + 2) / 2 && m_x <= glyph_w * 20 + 2) {
              shiftdate(&cal_m, &cal_y, 1);
              XClearWindow(display, calendar);
              paintCalendar(display, cal_m, cal_y, caldraw, calfont, &xftcolor, &xfthlcolor);
            }
          }
          XFlush(display);
        }
        if (event.type == Expose) {
          XClearWindow(display, dockapp);
          if (cal && caldraw != NULL && calfont != NULL && calendar != (Window)NULL) {
            XClearWindow(display, calendar);
            paintCalendar(display, cal_m, cal_y, caldraw, calfont, &xftcolor, &xfthlcolor);
          }
          XftDrawString8(draw, &xftcolor, font, x, y, (unsigned char*)strtime, strlen(strtime));
          XFlush(display);
        }
      }
      break;
    }
  }

  XftFontClose(display, font);
  if (cal) {
    XftFontClose(display, calfont);
    XftDrawDestroy(caldraw);
  }
  XftDrawDestroy(draw);
  XCloseDisplay(display);

  return 0;
}
