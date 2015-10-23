/* demo3.c	- requires GKrellM 2.0.0 or better
|		gcc -fPIC `pkg-config gtk+-2.0 --cflags` -c demo3.c
|		gcc -shared -Wl -o demo3.so demo3.o
|		gkrellm -p demo3.so
|
|  This is a demo of the various ways of making buttons in GKrellM.  The
|  main differences with respect to buttons for version 1.2.x are:
|    * There is a new more versatile gkrellm_make_scaled_button().
|    * There is no longer the constraint to turn text or pixmap decals into
|      buttons after panels are created.  The decals may be turned into
|      buttons immediately after the decals are created.
|
|  A button image should contain at least two frames stacked on top of
|  each other.  Any of the frames may be specified to be the in or pressed
|  frame.  This differs from the traditional way to create button images
|  which requires the creation of two separate images where the names of the
|  images determine which is the out and which is the pressed.  GKrellM
|  handles buttons this way because buttons are an extension of the decal
|  object which can have multiple frames for efficient animation purposes.
|  Consequently, buttons may be similarly animated (or even moved) with
|  multiple out images possible.
|
|  WIN32 defines in this demo are for portability to Windows.
*/

#if !defined(WIN32)
#include <gkrellm2/gkrellm.h>
#else
#include <src/gkrellm.h>
#include <src/win32-plugin.h>
#endif


  /* CONFIG_NAME would be the name in the configuration tree, but this demo
  |  does not have a config.  See demo1.c for that.
  */
#define	CONFIG_NAME	"Demo3"

  /* STYLE_NAME will be the theme subdirectory for custom images for this
  |  plugin and it will be the gkrellmrc style name for custom settings.
  */
#define	STYLE_NAME	"demo3"

static GkrellmMonitor *monitor;
static GkrellmPanel   *panel;

static GkrellmDecal *text1_decal,
                    *text2_decal,
                    *pixmap_decal;

static gchar    *scroll_text = "Some scrolling text.  ";

static gint     style_id;

static gint     button_state;		/* For text2_decal button */
static gchar    *button_text[2]	= {"Hello", "Bye" };

#if defined(GKRELLM_HAVE_DECAL_SCROLL_TEXT)
static gboolean		scroll_loop_mode;
#endif


static void
cb_button(GkrellmDecalbutton *button, gpointer data)
	{
	if (GPOINTER_TO_INT(data) == 0)
		{
		printf("Large scaled button pressed.\n");
#if defined(GKRELLM_HAVE_DECAL_SCROLL_TEXT)
		/* Demonstrate the scroll looping mode.
		*/
		printf("Toggling horizontal loop mode.\n");
		scroll_loop_mode = !scroll_loop_mode;
		scroll_text = scroll_loop_mode
					? "Some scrolling text -- "
					: "Some scrolling text";

		gkrellm_decal_scroll_text_horizontal_loop(text1_decal,
					scroll_loop_mode);
#endif
		}
	else if (GPOINTER_TO_INT(data) == 1)
		printf("Small auto-hiding scaled button pressed\n");
	else if (GPOINTER_TO_INT(data) == 2)
		{
		printf("button for text2_decal pressed\n");
		button_state = !button_state;

		/* Draw new text on the button.  For gkrellm versions < 2.2.0, decal
		|  drawing occurs only if the decal "value" (the last argument)
		|  changes.  The "value" is ignored in gkrellm versions >= 2.2.0
		|  and text is internally checked for changes before being redrawn.
		|  To be compatible with all versions, just pass the index of the
		|  text string we draw because it will change each time a different
		|  string is drawn.
		*/
		gkrellm_draw_decal_text(panel, text2_decal,
					button_text[button_state], button_state);

		/* Decal draws don't show up until gkrellm_draw_panel_layers() is
		|  called.  This is done in update_plugin(), so doing it here is only
		|  useful if the update_plugin() updates infrequently and we don't
		|  want a delay before the change is seen.  So for this demo, it does
		|  not need to be done here, but I do it anyway for emphasis.
		*/
		gkrellm_draw_panel_layers(panel);
		}
	else if (GPOINTER_TO_INT(data) == 3)
		printf("button for pixmap_decal pressed\n");
	}

static gint
panel_expose_event(GtkWidget *widget, GdkEventExpose *ev)
	{
	gdk_draw_pixmap(widget->window,
			widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
			panel->pixmap, ev->area.x, ev->area.y, ev->area.x, ev->area.y,
			ev->area.width, ev->area.height);
	return FALSE;
	}


static void
update_plugin()
	{
	gint		w_scroll, w_decal;
	static gint	x_scroll;

#if defined(GKRELLM_HAVE_DECAL_SCROLL_TEXT)
	/* Gkrellm version 2.2.0 adds a scrolling text mode to a text decal so
	|  that inherently slow Pango text draws can be avoided at each scroll
	|  step as long as the sroll text string is unchanged.
	*/
	gkrellm_decal_scroll_text_set_text(panel, text1_decal, scroll_text);
	gkrellm_decal_scroll_text_get_size(text1_decal, &w_scroll, NULL);
	gkrellm_decal_get_size(text1_decal, &w_decal, NULL);
	x_scroll -= 1;
	if (x_scroll <= -w_scroll)
		x_scroll = scroll_loop_mode ? 0 : w_decal;
	gkrellm_decal_text_set_offset(text1_decal, x_scroll, 0);
#else
	w_decal = text1_decal->w;
	w_scroll = gdk_string_width(text1_decal->text_style.font, scroll_text);
	x_scroll -= 1;
	if (x_scroll <= -w_scroll)
		x_scroll = w_decal;
	text1_decal->x_off = x_scroll;
	gkrellm_draw_decal_text(panel, text1_decal, scroll_text, x_scroll);
#endif

	gkrellm_draw_panel_layers(panel);
	}


static void
create_plugin(GtkWidget *vbox, gint first_create)
	{
	GkrellmDecalbutton	*button;
	GkrellmStyle	*style;
	GkrellmTextstyle *ts, *ts_alt;
	GdkPixmap		*pixmap;
	GdkBitmap		*mask;
	gint			y;
	gint			x;

	/* See comments about first create in demo2.c
	*/
	if (first_create)
		panel = gkrellm_panel_new0();

	style = gkrellm_meter_style(style_id);

	/* Each Style has two text styles.  The theme designer has picked the
	|  colors and font sizes, presumably based on knowledge of what you draw
	|  on your panel.  You just do the drawing.  You can assume that the
	|  ts font is larger than the ts_alt font.
	*/
	ts = gkrellm_meter_textstyle(style_id);
	ts_alt = gkrellm_meter_alt_textstyle(style_id);


	/* ==== Create a text decal that will be used to scroll text. ====
	|  Make it the full panel width (minus the margins).  Position it at top
	|  border and left margin of the style.  The "Ay" string is a vertical
	|  sizing string for the decal and not an initialization string.
	*/
	text1_decal = gkrellm_create_decal_text(panel, "Ay", ts, style,
				-1,     /* x = -1 places at left margin */
				-1,     /* y = -1 places at top margin	*/
				-1);    /* w = -1 makes decal the panel width minus margins */
	y = text1_decal->y + text1_decal->h + 2;


	/* ==== Create a scaled button ====
	|  This is the easiest and most versatile way to create a button and is
	|  a new function for GKrellM 2.0.0.  Here we use the builtin default
	|  button image which is a simple 2 frame in/out image.  If you supply
	|  your own image, there can be as many frames as you wish which can be
	|  used for state indicating purposes.  Make two buttons, one large and one
	|  small to demonstrate scaling capability.  Make the small one auto-hide.
	|  See demo4 for a more complicated scaled button which has an irregular
	|  shape and a custom in_button callback to detect when the mouse is on
	|  a non-transparent part of the button.
	*/
	button = gkrellm_make_scaled_button(panel,
				NULL,               /* GkrellmPiximage image to use to   */
				                    /*   create the button images. Use a */
				                    /*   builtin default if NULL         */
				cb_button,          /* Button clicked callback function  */
				GINT_TO_POINTER(0), /* Arg to callback function          */
				FALSE,              /* auto_hide: if TRUE, button is visible */
				                    /*   only when mouse is in the panel */
				FALSE,              /* set_default_border: if TRUE, apply a */
				                    /*   default border of 1,1,1,1.  If false*/
				                    /*   use the GkrellmPiximage border which*/
				                    /*   in this case is 0,0,0,0         */
				0,                  /* Image depth if image != NULL      */
				0,                  /* Initial out frame if image != NULL */
				0,                  /* Pressed frame if image != NULL    */
				2,                  /* x position of button  */
				y,                  /* y position of button  */
				13,                 /* Width for scaling the button  */
				15);                /* Height for scaling the button */

	x = button->decal->x + button->decal->w + 2;
	button = gkrellm_make_scaled_button(panel, NULL, cb_button,
				GINT_TO_POINTER(1), TRUE, FALSE, 0, 0, 0,
				x, y, 6, 8);


	/* ==== Create a text decal and convert it into a decal button. ====
	|  Text decals are converted into buttons by being put into a meter or
	|  panel button.  This "put" overlays the text decal with special button
	|  in and out images that have a transparent interior and a non-transparent
	|  border. The "Hello" string is not an initialization string, it is just
	|  a vertical sizing string.  After the decal is created, draw the
	|  initial text onto the decal.
	*/
	x = button->decal->x + button->decal->w + 2;
	text2_decal = gkrellm_create_decal_text(panel, "Hello", ts_alt, style,
				x,
				y,      /* Place below the scrolling text1_decal     */
				0);     /* w = 0 makes decal the sizing string width */

	gkrellm_put_decal_in_meter_button(panel, text2_decal,
				cb_button,          /* Button clicked callback function */
				GINT_TO_POINTER(2), /* Arg to callback function      */
				NULL);              /* Optional margin struct to pad the size */

	gkrellm_draw_decal_text(panel, text2_decal, button_text[button_state],
				button_state);


	/* ==== Create a pixmap decal and convert it into a decal button ====
	|  Pixmap decals are directly converted into buttons (no put operation)
	|  because pixmap decals have frames which will provide the in and out
	|  button images.  A plugin custom image may be loaded and rendered to
	|  a pixmap to be used for the button, but here I'm using the pre-loaded
	|  pixmap from the builtin decal_misc.xpm image.  First make a decal out
	|  of the pixmap.  Then pass the decal frames we want to be the in/out
	|  images when we make the decal button.
	*/
	pixmap = gkrellm_decal_misc_pixmap();
	mask = gkrellm_decal_misc_mask();
	x = text2_decal->x + text2_decal->w + 2;

	pixmap_decal = gkrellm_create_decal_pixmap(panel, pixmap, mask,
				N_MISC_DECALS, NULL, x, y);

	gkrellm_make_decal_button(panel, pixmap_decal,
				cb_button,          /* Button clicked callback function */
				GINT_TO_POINTER(3), /* Arg to callback function */
				D_MISC_BUTTON_OUT,  /* Button out (not pressed) frame */
				D_MISC_BUTTON_IN);  /* Button pressed frame */


	/* Configure the panel to hold the above created decals, and create it.
	*/
	gkrellm_panel_configure(panel, NULL, style);
	gkrellm_panel_create(vbox, monitor, panel);

	/* Note: the above gkrellm_draw_decal_text() call will not
	|  appear on the panel until a gkrellm_draw_panel_layers() call is
	|  made.  This will be done in update_plugin(), otherwise we would
	|  make the call here after the panel is created and anytime the
	|  decals are changed.
	*/

	if (first_create)
	    g_signal_connect(G_OBJECT (panel->drawing_area), "expose_event",
    	        G_CALLBACK(panel_expose_event), NULL);
	}


/* The monitor structure tells GKrellM how to call the plugin routines.
*/
static GkrellmMonitor	plugin_mon	=
	{
	CONFIG_NAME,        /* Name, for config tab.    */
	0,                  /* Id,  0 if a plugin       */
	create_plugin,      /* The create function      */
	update_plugin,      /* The update function      */
	NULL,               /* The config tab create function   */
	NULL,               /* Apply the config function        */

	NULL,               /* Save user config   */
	NULL,               /* Load user config   */
	NULL,               /* config keyword     */

	NULL,               /* Undefined 2  */
	NULL,               /* Undefined 1  */
	NULL,               /* private      */

	MON_MAIL,           /* Insert plugin before this monitor */

	NULL,               /* Handle if a plugin, filled in by GKrellM     */
	NULL                /* path if a plugin, filled in by GKrellM       */
	};


  /* All GKrellM plugins must have one global routine named init_plugin()
  |  which returns a pointer to a filled in monitor structure.
  */
#if defined(WIN32)
__declspec(dllexport) GkrellmMonitor *
gkrellm_init_plugin(win32_plugin_callbacks* calls)
#else
GkrellmMonitor *
gkrellm_init_plugin(void)
#endif
{
	/* If this call is made, the background and krell images for this plugin
	|  can be custom themed by putting bg_meter.png or krell.png in the
	|  subdirectory STYLE_NAME of the theme directory.  Text colors (and
	|  other things) can also be specified for the plugin with gkrellmrc
	|  lines like:  StyleMeter  STYLE_NAME.textcolor orange black shadow
	|  If no custom themeing has been done, then all above calls using
	|  style_id will be equivalent to style_id = DEFAULT_STYLE_ID.
	*/
	style_id = gkrellm_add_meter_style(&plugin_mon, STYLE_NAME);
	monitor = &plugin_mon;
	return &plugin_mon;
	}
