/* demo5	- requires GKrellM 2.2.0 or better
|
|	This is the gkrellm normal or client plugin part of demo5.
*/

#include <gkrellm2/gkrellm.h>

#include "demo5-common.h"

  /* A plugin can grab a debug value to use for testing.  Pick some random
  |  value and hope it doesn't collide with some other plugin.  Then run
  |  gkrellm -debug99 to do the debugging.
  */
#define	DEMO5_DEBUG_VALUE	99


  /* DEMO5_CONFIG_NAME would be the name in the configuration tree.
  |  Not used in this demo.  See other demos.
  */
#define	DEMO5_CONFIG_NAME	"Demo5"


#define	KRELL_FULL_SCALE	59

static GkrellmMonitor	*monitor;
static GkrellmPanel		*panel;
static GkrellmKrell		*krell;
static GkrellmTicks		*pGK;
static gint				style_id;
static gboolean			demo5_need_update;

  /* Data collection is done via a function pointer set below.
  */
static void			(*demo5_get_data)(Demo5Data *data);

static Demo5Data	demo5_data;


/* ---------- Client mode interface to demo5d server data  -------- */
#if defined(GKRELLM_HAVE_CLIENT_MODE_PLUGINS)

static gboolean		demo5d_server_available;

static gint			demo5d_version_major,
					demo5d_version_minor,
					demo5d_version_rev;


  /* When in client mode and the gkrellm_init_plugin() function switches to
  |  getting demo5d server data, this function will be called when demo5d
  |  data lines are received.  The idea is to save the data into a cached
  |  state so a gkrellm plugin can read it asynchronously at the current
  |  gkrellm update rate.
  |  A gkrellmd server plugin should initially send a complete set of data
  |  values but may subsequently send only changed values.
  */
static struct
	{
	gint		data;
	gchar		label[32];
	}
	demo5d_cache;

  /* Save data received from the server.
  */
static void
demo5_client_data_from_server(gchar *line)
	{
	gchar	which[32], item[32];

	if (gkrellm_plugin_debug() == DEMO5_DEBUG_VALUE)
		printf("demo5_data_from_server: %s\n", line);

	sscanf(line, "%31s %31s", which, item);

	if (!strcmp(which, "value"))
		demo5d_cache.data = atoi(item);
	else if (!strcmp(which, "label"))
		strcpy(demo5d_cache.label, item);
	}

  /* Read cached data as requested from the update_plugin() function.  When
  |  gkrellm is run in client mode, there is an initial update phase which
  |  will have initially called the above demo5_client_data_from_server()
  |  before the first update_plugin() call.  So data can be guaranteed to
  |  be available on the first call.
  */
static void
demo5_client_data_read(Demo5Data *data)
	{
	data->value = demo5d_cache.data;
	g_free(data->label);
	data->label = g_strdup(demo5d_cache.label);
	}
#endif


/* ----------------- Gkrellm plugin --------------- */

static void
update_plugin()
    {
	if (!pGK->five_second_tick && !demo5_need_update)
		return;
	demo5_need_update = FALSE;

	/* Collect data from either a server or the local machine.  See below.
	*/
	(*demo5_get_data)(&demo5_data);

	gkrellm_update_krell(panel, krell, demo5_data.value);

	/* the demo5_data.label data is not yet displayed in this demo... */

	gkrellm_draw_panel_layers(panel);

	/* If for some reason a client plugin needs to send data to its server
	|  plugin, this call may be made.  Remember, if data is sent to modify
	|  server plugin behavior, that the server may be serving data to
	|  multiple clients.
	*/
#if defined(GKRELLM_HAVE_CLIENT_MODE_PLUGINS)
#if GKRELLM_CHECK_VERSION(2,2,5)
	if (pGK->minute_tick && demo5d_server_available)
		gkrellm_client_send_to_server(DEMO5_SERVE_NAME,
					"Hello demo5d server, here's data for you: pie=good");
#endif
#endif
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

	if (first_create)
		panel = gkrellm_panel_new0();

	style = gkrellm_meter_style(style_id);
	krell_image = gkrellm_krell_meter_piximage(style_id);
	krell = gkrellm_create_krell(panel, krell_image, style);

	gkrellm_monotonic_krell_values(krell, FALSE);

	gkrellm_set_krell_full_scale(krell, KRELL_FULL_SCALE, 1);

#if defined(GKRELLM_HAVE_CLIENT_MODE_PLUGINS)
	if (!demo5d_server_available && gkrellm_client_mode())
		gkrellm_panel_configure(panel, "local: demo5", style);
	else
		gkrellm_panel_configure(panel, "demo5", style);
#else
	gkrellm_panel_configure(panel, "demo5", style);
#endif

	gkrellm_panel_create(vbox, monitor, panel);

	if (first_create)
	    g_signal_connect(G_OBJECT (panel->drawing_area), "expose_event",
    	        G_CALLBACK(panel_expose_event), NULL);

	demo5_need_update = TRUE;
	}


/* The monitor structure tells GKrellM how to call the plugin routines.
*/
static GkrellmMonitor	demo5_mon	=
	{
	DEMO5_CONFIG_NAME,  /* Title for config clist.   */
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


  /* Client mode setup: A gkrellmd server plugin can transmit initialization
  |  or status information during the gkrellm client setup phase and those
  |  lines are parsed here.  This function will be called only if gkrellm is
  |  run in client mode and the matching gkrellmd plugin is loaded.
  |  For each gkrellmd plugin loaded, there will automatically be generated
  |  an "available" setup line so a plugin can know if to switch into
  |  client mode.  Then an enabled plugin can either display local or
  |  server data when gkrellm is run in client mode.  It might be
  |  useful for gkrellm client mode capable plugins to somehow indicate
  |  if their gkrellmd plugin counterpart is not loaded and there is a fallback
  |  to displaying localhost derived data.  The above panel label does it
  |  for this demo5, but it could be something else like a text color
  |  change, an indication in a tooltip, or something else.
  |  Alternatively, the plugin could decide to not load if gkrellm is run in
  |  client mode and there is no gkrellmd plugin. ie return NULL in
  |  gkrellm_init_plugin() if (!server_available && gkrellm_client_mode()).
  |
  |  The server demo5d simply transmits its version numbers as an example of
  |  some possibly useful setup values.
  */
#if defined(GKRELLM_HAVE_CLIENT_MODE_PLUGINS)
static void
demo5_client_setup(gchar *line)
	{
	if (gkrellm_plugin_debug() == DEMO5_DEBUG_VALUE)
		printf("demo5_client_setup: %s\n", line);

	if (!strcmp(line, "available"))
		demo5d_server_available = TRUE;
	else if (!strncmp(line, "version ", 8))
		sscanf(line, "%*s %d %d %d", &demo5d_version_major,
					&demo5d_version_minor, &demo5d_version_rev);
	}
#endif


  /* All GKrellM plugins must have one global routine named
  |  gkrellm_init_plugin() which returns a pointer to a filled
  |  in monitor structure.
  */
GkrellmMonitor *
gkrellm_init_plugin(void)
	{
	pGK = gkrellm_ticks();

	style_id = gkrellm_add_meter_style(&demo5_mon, DEMO5_STYLE_NAME);

	if (gkrellm_plugin_debug() == DEMO5_DEBUG_VALUE)
		printf("demo5: gkrellm_init_plugin()\n");

#if defined(GKRELLM_HAVE_CLIENT_MODE_PLUGINS)
	/* If gkrellm is run in client mode and there is a demo5d plugin loaded
	|  on the server, read the setup lines from our demo5d server plugin.
	|  When this gkrellm_init_plugin() function is called in client mode,
	|  gkrellm will be connected to a server and will have already received
	|  and stored setup data for us to read.
	*/
	gkrellm_client_plugin_get_setup(DEMO5_SERVE_NAME, demo5_client_setup);

	if (gkrellm_plugin_debug() == DEMO5_DEBUG_VALUE)
		printf("demo5: have client mode plugins\n");

	/* Select data reading function pointer to read gkrellmd server data lines
	|  or to read local machine data using the data reading function in data.c
	|  If reading from a server, connect a function to receive the data lines
	|  sent by our server plugin.
	*/
	if (demo5d_server_available)
		{
		if (gkrellm_plugin_debug() == DEMO5_DEBUG_VALUE)
			printf("demo5: demo5d server is available.\n");

		demo5_get_data = demo5_client_data_read;
		gkrellm_client_plugin_serve_data_connect(&demo5_mon,
				DEMO5_SERVE_NAME, demo5_client_data_from_server);
		}
	else
		{
		if (gkrellm_plugin_debug() == DEMO5_DEBUG_VALUE)
			printf("demo5: demo5d server is not available.\n");

		demo5_get_data = demo5_data_read;
		}
#else
	if (gkrellm_plugin_debug() == DEMO5_DEBUG_VALUE)
		printf("demo5: no client mode.  Is gkrellm >= 2.2.0 installed?\n");

	demo5_get_data = demo5_data_read;
#endif

	monitor = &demo5_mon;
	return &demo5_mon;
	}
