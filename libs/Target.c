/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
** fvwmlib_get_target_window and fvwmlib_keyboard_shortcuts - handle window
** selection from modules and fvwm.
*/

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include <X11/keysym.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/cursorfont.h>

#include "fvwmlib.h"

void fvwmlib_keyboard_shortcuts(
    Display *dpy, int screen, XEvent *Event, int x_move_size, int y_move_size,
    int ReturnEvent)
{
  int x;
  int y;
  int x_root;
  int y_root;
  int x_move;
  int y_move;
  KeySym keysym;
  Window JunkRoot;
  unsigned int JunkMask;

  if (y_move_size < 5)
    y_move_size = 5;
  if (x_move_size < 5)
    x_move_size = 5;
  if(Event->xkey.state & ControlMask)
    x_move_size = y_move_size = 1;
  if(Event->xkey.state & ShiftMask)
    x_move_size = y_move_size = 100;

  keysym = XLookupKeysym(&Event->xkey,0);

  x_move = 0;
  y_move = 0;
  switch(keysym)
    {
    case XK_Up:
    case XK_KP_8:
    case XK_k:
    case XK_p:
      y_move = -y_move_size;
      break;
    case XK_Down:
    case XK_KP_2:
    case XK_n:
    case XK_j:
      y_move = y_move_size;
      break;
    case XK_Left:
    case XK_KP_4:
    case XK_b:
    case XK_h:
      x_move = -x_move_size;
      break;
    case XK_Right:
    case XK_KP_6:
    case XK_f:
    case XK_l:
      x_move = x_move_size;
      break;
    case XK_KP_1:
      x_move = -x_move_size;
      y_move = y_move_size;
      break;
    case XK_KP_3:
      x_move = x_move_size;
      y_move = y_move_size;
      break;
    case XK_KP_7:
      x_move = -x_move_size;
      y_move = -y_move_size;
      break;
    case XK_KP_9:
      x_move = x_move_size;
      y_move = -y_move_size;
      break;
    case XK_Return:
    case XK_KP_Enter:
    case XK_space:
      /* beat up the event */
      Event->type = ReturnEvent;
      break;
    case XK_Escape:
      /* simple code to bag out of move - CKH */
      /* return keypress event instead */
      Event->type = KeyPress;
      Event->xkey.keycode = XKeysymToKeycode(Event->xkey.display,keysym);
      break;
    default:
      break;
    }
  XQueryPointer(dpy, RootWindow(dpy, screen), &JunkRoot, &Event->xany.window,
		&x_root, &y_root, &x, &y, &JunkMask);

  if((x_move != 0)||(y_move != 0))
    {
      /* beat up the event */
      XWarpPointer(dpy, None, RootWindow(dpy, screen), 0, 0, 0, 0,
                   x_root+x_move, y_root+y_move);

      /* beat up the event */
      Event->type = MotionNotify;
      Event->xkey.x += x_move;
      Event->xkey.y += y_move;
      Event->xkey.x_root += x_move;
      Event->xkey.y_root += y_move;
    }
}

void fvwmlib_get_target_window(
    Display *dpy, int screen, char *MyName, Window *app_win,
    Bool return_subwindow)
{
  XEvent eventp;
  int val = -10,trials;
  Bool finished = False;
  Bool canceled = False;
  Window Root = RootWindow(dpy, screen);

  trials = 0;
  while((trials <100)&&(val != GrabSuccess))
    {
      val=XGrabPointer(dpy, Root, True,
		       ButtonReleaseMask,
		       GrabModeAsync, GrabModeAsync, Root,
		       XCreateFontCursor(dpy,XC_crosshair),
		       CurrentTime);
      if(val != GrabSuccess)
	{
	  usleep(1000);
	}
      trials++;
    }
  if(val != GrabSuccess)
    {
      fprintf(stderr,"%s: Couldn't grab the cursor!\n",MyName);
      exit(1);
    }
  XGrabKeyboard(dpy, Root, True, GrabModeAsync, GrabModeAsync, CurrentTime);

  while (!finished && !canceled)
  {
    XMaskEvent(dpy, ButtonReleaseMask | KeyReleaseMask, &eventp);
    if(eventp.type == KeyRelease)
    {
      KeySym keysym = XLookupKeysym(&eventp.xkey,0);
      switch (keysym)
      {
      case XK_Escape:
        canceled = True;
        break;
      case XK_space:
      case XK_Return:
      case XK_KP_Enter:
        finished = True;
        break;
      default:
        fvwmlib_keyboard_shortcuts(dpy, screen, &eventp, 0, 0, 0);
        break;
      }
    }
    else
    {
      finished = True;
    }
  }

  XUngrabKeyboard(dpy, CurrentTime);
  XUngrabPointer(dpy, CurrentTime);
  XSync(dpy,0);
  if (canceled)
  {
    *app_win = None;
    return;
  }
  *app_win = eventp.xany.window;
  if(return_subwindow && eventp.xbutton.subwindow != None)
    *app_win = eventp.xbutton.subwindow;

  return;
}
