/* demo1.c	- requires GKrellM 2.0.0 or better
|     gcc -fPIC `pkg-config gtk+-2.0 --cflags` -c demo1.c
|     gcc -shared -Wl -o demo1.so demo1.o
|     gkrellm -p demo1.so
|
|  This is a demo of making a chart in GKrellM.  It also shows creating
|  a configuration page and saving and loading configuration values.
|
|  WIN32 defines in this demo are for portability to Windows.
*/

#if !defined(WIN32)
#include <gkrellm2/gkrellm.h>
#else
#include <src/gkrellm.h>
#include <src/win32-plugin.h>
#endif



#define CONFIG_NAME             "Demo1-Chart"
#define MONITOR_CONFIG_KEYWORD  "demo1"
#define STYLE_NAME              "demo1"

#define	MIN_GRID_RES    2
#define	MAX_GRID_RES    50

static GkrellmMonitor      *mon;
static GkrellmChart        *chart;
static GkrellmChartdata    *plugin_cd;
static GkrellmChartconfig  *chart_config;
static GkrellmTicks        *pGK;

static gint	plugin_enable;
static gint	plugin_style_id;


static gint
chart_expose_event(GtkWidget *widget, GdkEventExpose *ev)
	{
	gdk_draw_pixmap(widget->window,
			widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
			chart->pixmap, ev->area.x, ev->area.y, ev->area.x, ev->area.y,
			ev->area.width, ev->area.height);
	return FALSE;
	}

static void
cb_chart_click(GtkWidget *widget, GdkEventButton *event, gpointer data)
	{
	if (event->button == 3)
		gkrellm_chartconfig_window_create(chart);
	}

  /* This routine is needed only if you want to draw something extra on the
  |  chart besides the raw data and grid lines that are drawn automatically
  |  in gkrellm_draw_chartdata().  If you have nothing extra to draw, don't
  |  connect to this routine with gkrellm_set_draw_chart_function().
  |  Here I'm drawing an extra centered "Sawtooth" label.
  */
static void
draw_plugin_chart(void)
	{
	gkrellm_draw_chartdata(chart);
	gkrellm_draw_chart_text(chart, plugin_style_id, "\\f\\cSawtooth");
	gkrellm_draw_chart_to_screen(chart);
	}

static void
update_plugin(void)
	{
	static gint	sawtooth;

	if (pGK->second_tick)
		{
		/* Make up some data to store on the chart.
		*/
		if (plugin_enable)
			sawtooth = 0;
		else if (++sawtooth > 20)
			sawtooth = 0;
		gkrellm_store_chartdata(chart, 0, sawtooth * 2);

		/* If you don't have a separate draw chart function, then put
		|  here gkrellm_draw_chartdata() and gkrellm_draw_chart_to_screen().
		*/
		draw_plugin_chart();
		}
	}

static void
cb_scale_chart(GkrellmChartconfig *cf, gpointer data)
	{
	/* If you have any work to do if the chart number of grids mode or
	|  resolution per grid changes, do it here.  These can change because of
	|  auto scaling or the user changing the chartconfig.  If you have
	|  nothing to do, then omit this routine and don't connect to it
	|  in the create_plugin().
	*/
	printf("Chart grid resolution or number of grids mode is changed:\n");
	printf("New number of grids mode=%d, new grid resolution=%d\n",
			gkrellm_get_chartconfig_fixed_grids(cf),
			gkrellm_get_chartconfig_grid_resolution(cf));

	/* The scalemax won't be updated until gkrellm_draw_chartdata() is called.
	|  So if in auto number of grids mode, we won't know how many grids will
	|  be drawn until then.
	*/
	printf("Old scalemax=%d\n\n",
			gkrellm_get_chart_scalemax(chart));
	}

static void
create_plugin(GtkWidget *vbox, gint first_create)
	{
	if (first_create)
		chart = gkrellm_chart_new0();

	/* Chart heights initially default to 40 if this next call is not made.
	|  Once the user configs the chart, this call has no effect.  Make this
	|  call before the chart is created.
	*/
	gkrellm_set_chart_height_default(chart, 20);

	/* The address of a ChartConfig struct pointer must be passed to the create
	|  function.  If the pointer is NULL, a ChartConfig struct will be
	|  allocated and the pointer updated.  But usually the pointer will not
	|  be NULL if you load a saved chartconfig in load_plugin_config().
	*/
	gkrellm_chart_create(vbox, mon, chart, &chart_config);

	/* This chart will have only one data set drawn on it, but you may add
	|  multiple data sets to be drawn.
	*/
	plugin_cd = gkrellm_add_default_chartdata(chart, "Plugin Data");

	/* If the data to be charted monotonically increases, then don't make
	|    this next call.  The default is data is assumed to monotonically
	|    increase and difference values are automatically charted.
	*/
	gkrellm_monotonic_chartdata(plugin_cd, FALSE);

	/* Setting the draw style default also has no effect once the user has
	|    configured the chart.
	|  If you set the CHARTDATA_ALLOW_HIDE flag, there will be a button in
	|    the chart config window allowing the user to hide the data.
	|  Make these calls after adding a chartdata.
	*/
	gkrellm_set_chartdata_draw_style_default(plugin_cd, CHARTDATA_LINE);
	gkrellm_set_chartdata_flags(plugin_cd, CHARTDATA_ALLOW_HIDE);

	/* Set your own chart draw function if you have extra info to draw
	*/
	gkrellm_set_draw_chart_function(chart, draw_plugin_chart, NULL);

	/* You can connect to chart scaling changes in case you need to adjust
	|  krell scaling or something else.
	*/
	gkrellm_chartconfig_fixed_grids_connect(chart_config,
				cb_scale_chart, NULL);
	gkrellm_chartconfig_grid_resolution_connect(chart_config,
				cb_scale_chart, NULL);

	/* If this next call is made, then there will be a resolution spin
	|  button on the chartconfig window so the user can change resolutions.
	*/
	gkrellm_chartconfig_grid_resolution_adjustment(chart_config, TRUE,
			0, (gfloat) MIN_GRID_RES, (gfloat) MAX_GRID_RES, 0, 0, 0, 70);
	gkrellm_chartconfig_grid_resolution_label(chart_config,
			"Units drawn on the chart");

	gkrellm_alloc_chartdata(chart);

	if (first_create)
		{
		g_signal_connect(G_OBJECT(chart->drawing_area),
				"expose_event", G_CALLBACK(chart_expose_event), NULL);
		g_signal_connect(G_OBJECT(chart->drawing_area),
				"button_press_event", G_CALLBACK(cb_chart_click), NULL);
		}
	else
		draw_plugin_chart();
	}



/* ---- User Config ---- */

static GtkWidget	*plugin_enable_button;

  /* Save any configuration data we have in config lines in the format:
  |  MONITOR_CONFIG_KEYWORD  config_keyword  data
  */
static void
save_plugin_config(FILE *f)
	{
	fprintf(f, "%s enable %d\n", MONITOR_CONFIG_KEYWORD, plugin_enable);

	/* Save any chart config changes the user has made.
	*/
	gkrellm_save_chartconfig(f, chart_config, MONITOR_CONFIG_KEYWORD, NULL);
	}

  /* When GKrellM is started up, load_plugin_config() is called if any
  |  config lines for this plugin are found.  The lines must have been
  |  saved by save_plugin_config().  gkrellm_load_chartconfig() must
  |  have the address of a ChartConfig struct pointer.  At this point, the
  |  pointer is almost always NULL and the function will allocate a
  |  ChartConfig struct and update the pointer.  The struct will be
  |  initialized with values from the config line.
  */
static void
load_plugin_config(gchar *config_line)
	{
	gchar	config_keyword[32], config_data[CFG_BUFSIZE];
	gint	n;

	if ((n = sscanf(config_line, "%31s %[^\n]",
				config_keyword, config_data)) != 2)
		return;
	if (!strcmp(config_keyword, "enable"))
		sscanf(config_data, "%d", &plugin_enable);
	else if (!strcmp(config_keyword, GKRELLM_CHARTCONFIG_KEYWORD))
		gkrellm_load_chartconfig(&chart_config, config_data, 1);
	}

  /* The apply is called whenever the user hits the OK or the Apply
  |  button in the config window.
  */
static void
apply_plugin_config(void)
	{
	plugin_enable = GTK_TOGGLE_BUTTON(plugin_enable_button)->active;
	}

static gchar	*plugin_info_text[] =
{
"<h>Setup Notes\n",
"Put any user instructions here.\n"
};

static void
create_plugin_tab(GtkWidget *tab_vbox)
	{
	GtkWidget	*tabs, *text;
	GtkWidget	*vbox, *vbox1;
	gint		i;

	/* Create your Gtk user config widgets here.
	*/
	tabs = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(tabs), GTK_POS_TOP);
	gtk_box_pack_start(GTK_BOX(tab_vbox), tabs, TRUE, TRUE, 0);

/* -- Options tab */
	vbox = gkrellm_gtk_framed_notebook_page(tabs, "Options");
	vbox1 = gkrellm_gtk_framed_vbox(vbox, "Various enables", 4, FALSE, 0, 2);
	gkrellm_gtk_check_button(vbox1, &plugin_enable_button, plugin_enable,
			FALSE, 0, "Enable something");

/* -- Info tab */
	vbox = gkrellm_gtk_framed_notebook_page(tabs, "Info");
	text = gkrellm_gtk_scrolled_text_view(vbox, NULL,
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	for (i = 0; i < sizeof(plugin_info_text)/sizeof(gchar *); ++i)
		gkrellm_gtk_text_view_append(text, plugin_info_text[i]);
	}

static GkrellmMonitor  plugin_mon  =
    {
	CONFIG_NAME,            /* Name, for config tab.        */
	0,                      /* Id,  0 if a plugin           */
	create_plugin,          /* The create_plugin() function */
	update_plugin,          /* The update_plugin() function */
	create_plugin_tab,      /* The create_plugin_tab() config function */
	apply_plugin_config,    /* The apply_plugin_config() function      */

	save_plugin_config,     /* The save_plugin_config() function  */
	load_plugin_config,     /* The load_plugin_config() function  */
	MONITOR_CONFIG_KEYWORD, /* config keyword                     */

	NULL,           /* Undefined 2  */
	NULL,           /* Undefined 1  */
	NULL,           /* private      */

	MON_MAIL,       /* Insert plugin before this monitor.       */
	NULL,           /* Handle if a plugin, filled in by GKrellM */
	NULL            /* path if a plugin, filled in by GKrellM   */
	};

#if defined(WIN32)
__declspec(dllexport) GkrellmMonitor *
gkrellm_init_plugin(win32_plugin_callbacks* calls)
#else
GkrellmMonitor *
gkrellm_init_plugin(void)
#endif
{
	/* This is a good place to initialize plugin variables, but you should
	|  not do anything that assumes your plugin will actually be running.
	|  This routine will be called for all plugins GKrellM finds, but if
	|  the user does not enable the plugin, nothing else will be called.
	*/

	pGK = gkrellm_ticks();
	plugin_style_id = gkrellm_add_chart_style(&plugin_mon, STYLE_NAME);
	mon = &plugin_mon;
	return &plugin_mon;
	}
