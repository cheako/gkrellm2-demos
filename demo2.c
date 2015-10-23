/* demo2.c	- requires GKrellM 2.0.0 or better
|		gcc -fPIC `pkg-config gtk+-2.0 --cflags` -c demo2.c
|		gcc -shared -Wl -o demo2.so demo2.o
|		gkrellm -p demo2.so
|
|  This is a demo of making a krell in GKrellM
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
#define	CONFIG_NAME	"Demo2"

  /* STYLE_NAME will be the theme subdirectory for custom images for this
  |  plugin and it will be the gkrellmrc style name for custom settings.
  */
#define	STYLE_NAME	"demo2"

  /* This demo just makes up some data for the krell to show.
  */
#define	KRELL_FULL_SCALE	30

static GkrellmMonitor *monitor;
static GkrellmPanel   *panel;
static GkrellmKrell   *krell;
static GkrellmTicks   *pGK;
static gint           style_id;

static void
update_plugin()
    {
	if ((pGK->timer_ticks % 2) == 0)
		{
		gkrellm_update_krell(panel, krell, pGK->timer_ticks % KRELL_FULL_SCALE);

		/* Updating a krell (or drawing a decal) draws only into local
		|  pixmaps.  After all krells and decals are "drawn", they must be
		|  really drawn to the screen with gkrellm_draw_panel_layers().
		*/
		gkrellm_draw_panel_layers(panel);
		}
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
create_plugin(GtkWidget *vbox, gint first_create)
	{
	GkrellmStyle    *style;
	GkrellmPiximage *krell_image;

	/* This create_plugin() routine is a create event routine which
	|  will be called when GKrellM is re-built at every theme or horizontal
	|  size change.  The idea is to allocate data structures and do some
	|  initialization only once (at first_create) and do decal and krell
	|  creation and plugin local image loading each time so theme changes
	|  will be picked up.
	|  Before this routine is called during the re-build, all of the krells
	|  and decals you previously created will have been destroyed.  However,
	|  for cases where a plugin wants to manage the decal and krell lists,
	|  it is possible to prevent the automatic krell and decal destruction.
	*/
	if (first_create)
		panel = gkrellm_panel_new0();

	/* Each plugin that has obtained a style_id (in gkrellm_init_plugin())
	|  can use a default krell that may have been custom themed.  Additional
	|  custom krells may be created (see demo4), but here we just use the
	|  default krell.  To create a krell, use our style (which has information
	|  about the krell depth, hot spot, and y offset into the panel), and our
	|  krell image.
	*/
	style = gkrellm_meter_style(style_id);
	krell_image = gkrellm_krell_meter_piximage(style_id);
	krell = gkrellm_create_krell(panel, krell_image, style);

	/* The default for GKrellM data update routines is to expect monotonically
	|  increasing data values.  So the real data values stored for display are
	|  differences between successive data updates.  But the data this demo
	|  krell will measure will be values within a fixed range, so we must
	|  turn off the monotonic mode.
	*/
	gkrellm_monotonic_krell_values(krell, FALSE);

	/* Set the krell full scale value and a krell scaling factor.  The scaling
	|  factor should almost always be 1.  Only chart styled krells that have
	|  a small full scale value might need a scaling factor greater than 1.
	*/
	gkrellm_set_krell_full_scale(krell, KRELL_FULL_SCALE, 1);

	/* Configuring a panel means to determine the height needed to accomodate
	|  its label and all of the created decals and krells that it contains.
	|  This is where we set the label on the panel to be "Plugin".  Many
	|  plugins will have panels with changing text or pixmap decals and will
	|  not use a label at all.  In those cases, just pass NULL for the label.
	|  Some plugins may want to have a fixed panel height.  Those should
	|  call gkrellm_panel_configure_set_height() instead of this:
	*/
	gkrellm_panel_configure(panel, "Plugin", style);

	/* Finally create the panel.  It will have a background image which will
	|  be the theme default or plugin specific if created by the theme maker.
	*/
	gkrellm_panel_create(vbox, monitor, panel);

	if (first_create)
	    g_signal_connect(G_OBJECT (panel->drawing_area), "expose_event",
    	        G_CALLBACK(panel_expose_event), NULL);
	}


/* The monitor structure tells GKrellM how to call the plugin routines.
*/
static GkrellmMonitor	plugin_mon	=
	{
	CONFIG_NAME,        /* Title for config clist.   */
	0,                  /* Id,  0 if a plugin       */
	create_plugin,      /* The create function      */
	update_plugin,      /* The update function      */
	NULL,               /* The config tab create function   */
	NULL,               /* Apply the config function        */

	NULL,               /* Save user config			*/
	NULL,               /* Load user config			*/
	NULL,               /* config keyword			*/

	NULL,               /* Undefined 2	*/
	NULL,               /* Undefined 1	*/
	NULL,               /* private	*/

	MON_MAIL,           /* Insert plugin before this monitor			*/

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
	pGK = gkrellm_ticks();

	/* If this next call is made, the background and krell images for this
	|  plugin can be custom themed by putting bg_meter.png or krell.png in the
	|  subdirectory STYLE_NAME of the theme directory.  Text colors (and
	|  other things) can also be specified for the plugin with gkrellmrc
	|  lines like:  StyleMeter  STYLE_NAME.textcolor orange black shadow
	|  If no custom theming has been done, then all above calls using
	|  style_id will be equivalent to style_id = DEFAULT_STYLE_ID.
	*/
	style_id = gkrellm_add_meter_style(&plugin_mon, STYLE_NAME);
	monitor = &plugin_mon;
	return &plugin_mon;
	}
