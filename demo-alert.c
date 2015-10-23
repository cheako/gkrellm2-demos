/* demo-alert.c	- requires GKrellM 2.0.0 or better
|		gcc -fPIC `pkg-config gtk+-2.0 --cflags` -c demo-alert.c
|		gcc -shared -Wl -o demo-alert.so demo-alert.o
|		gkrellm -p demo-alert.so
|
*/

/* Overview of GKrellM alerts:
|  A monitor should declare a GkrellmAlert pointer for each alert it wants.
|  An alert structure may exist independent of any GkrellmPanel, but when
|  alerts are triggered, they must be assigned to a panel.
|
|  1) After alerts are created, they may be initialized and activated with
|     function calls, but they are usually set up via an alert GUI config
|     window or by reading saved parameters from the user_config.  A monitor's
|     main configuraton page will typically have an "Alert" button with a
|     callback.  The sequence in the callback is:
|         a) Create the alert if does not already exist (alert pointer is NULL)
|            The alert will already exist if a previous alert GUI config
|            has been applied to it or if it was created when a previously
|            saved alert configuration was loaded at startup from the
|            user_config.
|         b) Call the gkrellm alert GUI config function on the alert.  The user
|            may then update the alert to new values or delete it.  If the
|            alert is deleted, the alert pointer will be set to NULL.
|  2) When an alert is created, it can have a GkrellmPanel pointer arg.
|     But, if an alert is created as a result of reading in a saved alert
|     configuration from the user_config at startup, there will be no
|     panels created yet.  Consequently, user configurable alerts should
|     always be created with a NULL panel pointer and connected to a callback
|     so a valid panel pointer may be assigned when the alert is triggered.
*/

/*
|  This demo can create 3 different alert variations and you can select
|  them by defining one of ALERT_CALLBACK1, ALERT_CALLBACK2, or ALERT_CALLBACK3
|  below.
|
|  For an example of creating a hardwired alert (does not have a user
|  config) see the alert created in hostname.c in the gkrellm source.
|  It uses gkrellm_alert_set_triggers() to hardwire the alert trigger limits.
|
*/

#include <gkrellm2/gkrellm.h>

/* Define only one of these
*/
#define ALERT_CALLBACK1
//#define ALERT_CALLBACK2
//#define ALERT_CALLBACK3


  /* To save myself some work for this demo, I'm shamelessly using the
  |  i8krellm prop image.  This image is used only for the ALERT_CALLBACK3
  |  case below.
  */
gchar *prop_anim_xpm[] = {
"18 108 7 1",
" 	c None",
".	c #A0A0A0",
"+	c #303030",
"@	c #808080",
"#	c #585858",
"$	c #800000",
"%	c #FF0000",
"                  ",
"        .+        ",
"       .@#+       ",
"       .@#+       ",
"       .@#+       ",
"        .+        ",
"        .+        ",
"        $$        ",
"       $%%$       ",
"       $%$$       ",
"        $$        ",
"        +.        ",
"        +.        ",
"       +#@.       ",
"       +#@.       ",
"       +#@.       ",
"        +.        ",
"                  ",
"                  ",
"                  ",
"             ..+  ",
"            .@#+  ",
"           .@@#+  ",
"           .@#+   ",
"          .@++    ",
"        $$#+      ",
"       $%%$       ",
"       $%$$       ",
"      +#$$        ",
"    ++@.          ",
"   +#@.           ",
"  +#@@.           ",
"  +#@.            ",
"  +..             ",
"                  ",
"                  ",
"                  ",
"                  ",
"                  ",
"                  ",
"                  ",
"                  ",
"                  ",
"  +++   $$   ...  ",
" +###++$%%$..@@@. ",
" .@@@..$%$$++###+ ",
"  ...   $$   +++  ",
"                  ",
"                  ",
"                  ",
"                  ",
"                  ",
"                  ",
"                  ",
"                  ",
"                  ",
"  +++             ",
"  .##+            ",
"  .@@#+           ",
"   .@#+           ",
"    ..#+          ",
"      .#$$        ",
"       $%%$       ",
"       $%$$       ",
"        $$#.      ",
"          +#..    ",
"           +#@.   ",
"           +#@@.  ",
"            +##.  ",
"             +++  ",
"                  ",
"                  ",
"                  ",
"        .+        ",
"       .@#+       ",
"       .@#+       ",
"       .@#+       ",
"        .+        ",
"        .+        ",
"  +++   $$   ...  ",
" +###++$%%$..@@@. ",
" .@@@..$%$$++###+ ",
"  ...   $$   +++  ",
"        +.        ",
"        +.        ",
"       +#@.       ",
"       +#@.       ",
"       +#@.       ",
"        +.        ",
"                  ",
"                  ",
"                  ",
"  +++        ..+  ",
"  .##+      .@#+  ",
"  .@@#+    .@@#+  ",
"   .@@+    .@#+   ",
"    ..#+  .@++    ",
"      .#$$#+      ",
"       $%%$       ",
"       $%$$       ",
"      +#$$#.      ",
"    ++@.  +#..    ",
"   +#@.    +@@.   ",
"  +#@@.    +#@@.  ",
"  +#@.      +##.  ",
"  +..        +++  ",
"                  ",
"                  "};



  /* CONFIG_NAME will be the name in the configuration tree.
  */
#define	CONFIG_NAME	"Alert-Demo1"

  /* STYLE_NAME will be the theme subdirectory for custom images for this
  |  plugin and it will be the gkrellmrc style name for custom settings.
  */
#define	STYLE_NAME	"alertdemo1"

#define CONFIG_KEYWORD	"alertdemo1"



  /* This demo will generate random numbers from 0 - MY_RANGE and an alert
  |  can be configured to trigger when the values are >= some user set limit.
  */
#define	MY_RANGE	20

static GkrellmMonitor *monitor;
static GkrellmPanel   *panel;
static GkrellmKrell   *krell;
static GkrellmDecal   *decal;
static GkrellmTicks   *pGK;
static gint           style_id;

static GkrellmAlert   *alert;

static void
update_plugin()
    {
	gint		value;
	static gint	holdoff;

	if (pGK->two_second_tick)
		{
		value = rand() % (MY_RANGE + 1);
		gkrellm_update_krell(panel, krell, value);

		/* If there is an alert configured, compare its configured limit
		|  values to our current random value and possibly trigger an alert
		*/
		gkrellm_check_alert(alert, value);

		/* To demonstrate controlling alert triggering, freeze the alert
		|  for 4 out of every 20 seconds.
		*/
		++holdoff;
		if ((holdoff % 10) > 7)
			{
			gkrellm_reset_alert(alert);
			gkrellm_freeze_alert(alert);
			}
		else
			gkrellm_thaw_alert(alert);

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



  /* Here's some variation in how the callback can be handled and shows
  |  increasing levels of control of the alert decal.
  |  Define an ALERT_CALLBACK above to see the different versions.
  */

#if defined(ALERT_CALLBACK1)
  /* This is the simplest callback and user configurable alerts should always
  |  do at least this.  When the alert is triggered, a panel is assigned to
  |  it and the default alert decal will appear in the panel scaled to fill
  |  the entire panel.
  */
static void
cb_alert_trigger(GkrellmAlert *alert, GkrellmPanel **p)
	{
	alert->panel = *p;
	}


#elif defined(ALERT_CALLBACK2)
  /* In this callback a panel is assigned and the size and position of the
  |  alert decal in the panel is specified.
  */
static void
cb_alert_trigger(GkrellmAlert *alert, GkrellmPanel **p)
	{
	GkrellmAlertdecal   *ad;

	alert->panel = *p;
	ad = &alert->ad;

	/* Set the alert decal to appear under the "Random" text decal and use
	|  the default built in alert decal.
	*/
	ad->x = decal->x + 1;
	ad->y = decal->y - 2;
	ad->w = decal->w - 1;
	ad->h = decal->h + 4;
	gkrellm_render_default_alert_decal(alert);
	}


#elif defined(ALERT_CALLBACK3)
  /* This shows how to set up an alert with your own warn and alarm trigger
  |  images.  Here, I'll place the alert decal to the right of the "Random"
  |  text decal.  The data for this callback is the address of a panel
  |  pointer because the alert may be created and connected to this callback
  |  before the panel is created (no valid panel pointer exists).
  */
static void
cb_alert_trigger(GkrellmAlert *alert, GkrellmPanel **p)
	{
	GkrellmAlertdecal      *ad;
	GkrellmPiximage        *im;
	static GkrellmPiximage *alarm_im;

	alert->panel = *p;
	ad = &alert->ad;

	/* Set the alert decal position in the panel.
	*/
	ad->x = decal->x + decal->w + 2;
	ad->y = 0;
	ad->w = (*p)->w - ad->x;
	ad->h = (*p)->h;

	/* Load my custom alert images.
	*/
	if (!alarm_im)
		{
		/* There probably should be separate alarm and warn images, but
		|  I'm too lazy here and have #included only one, so I'll use
		|  that for an alarm and use the builtin default for a warn.
		*/
		gkrellm_load_piximage(NULL, prop_anim_xpm, &alarm_im, NULL);
		}

	/* Select either the alarm or warn image.
	*/
	if (alert->high.alarm_on || alert->low.alarm_on)
		{
		im = alarm_im;
		ad->nframes = 6;
		}
	else
		gkrellm_get_decal_warn_piximage(&im, &ad->nframes);

	/* And load that image into the alert pixmap scaled to the size we want.
	*/
	gkrellm_scale_piximage_to_pixmap(im, &ad->pixmap, &ad->mask, ad->w,
			ad->h * ad->nframes);
	}
#endif



  /* Create an alert if one does not already exist.
  |  You can set up to handle alert triggers with varying degrees of control.
  |  For example:
  |  1) Give a non-NULL panel pointer and then don't connect with
  |     gkrellm_alert_trigger_connect().  Gkrellm will create a default
  |     panel sized alert decal for you.  But use this method for hardwired
  |     alerts and not user configurable alerts.
  |  2) Pass a NULL panel pointer and connect a callback which will set the
  |     right panel pointer at trigger time.  If only a panel is assigned at
  |     trigger time, gkrellm will make a default panel sized alert decal
  |     for you.
  |  3) Connect a callback which will set the panel pointer and/or render
  |     a custom size/position for the alert decal and/or set the pixmap.
  |     For this case, the alert decal may be a render of the default decal
  |     image or may be a custom image from the plugin.  The work needed is
  |     setting ad x,y,w,h and nframes and rendering to ad->pixmap.
  |
  |  The remainder of gkrellm_alert_create() args are:
  |  check_high: if TRUE, the alert config will allow setting a high limit
  |      value for triggering the alert when tested values are greater than
  |      or equal to the high limit.
  |  check_low: if TRUE, the alert config will allow setting a low limit
  |      value for triggering the alert when tested values are less than
  |      or equal to the low limit.
  |  do_updates: if TRUE, gkrellm will automatically draw the alert decal
  |      when the alert is triggered.  Set this to FALSE if the monitor
  |      update function draws the panel after checking the alert.
  |  max_high: the max value a high trigger can be set to.
  |  min_low:  the min value a low trigger can be set to.
  |  step0:    the step0 for the Gtk spin buttons that sets alert limits.
  |  step1:    the step1 for the Gtk spin buttons that sets alert limits.
  |  digits:   digits arg for the spin buttons.  Set this to your desired
  |            precision if checking float alert values.
  */
static void
create_alert(GkrellmAlert **alert)
	{
	if (!*alert)
		{
		/* Create a high alert that will trigger for test values higher
		|  than some limit.
		*/
		*alert = gkrellm_alert_create(
					NULL,           /* panel */
					"demo-alert",   /* Name of the alert      */
					"my units",     /* Units for alert values */
					TRUE,           /* check_high      */
					FALSE,          /* check_low       */
					TRUE,           /* do_updates      */
					MY_RANGE - 1,   /* max_high        */		
					1,              /* min_low         */
					1,              /* Gtk spin step0  */
					10,             /* Gtk spin step1  */
					0);             /* Gtk spin digits */
		}
	/* Set our callback for when alerts get triggered and set the
	|  callback data to be the address of our panel pointer.  The alert
	|  can be created from the config before the panel is created, so
	|  we can't just pass the pointer directly.  If the panel pointer was
	|  part of a larger plugin struct we could just pass that struct
	|  pointer and in the callbacks above reference something like
	|  mon->panel instead of the (*p)-> that is there now.
	*/
	gkrellm_alert_trigger_connect(*alert, cb_alert_trigger, &panel);
	}

  /* Callback when user clicks the "Alert" button in the config.
  |  Make sure an alert is created and then put up a config window for it.
  |  If the user does not actually set any trigger limit values or deletes
  |  the alert, then the alert will be set to NULL.  This is why we must
  |  check if the alert needs to be created each time in this callback.
  */
static void
cb_set_alert(GtkWidget *button, GkrellmPanel *p)
	{
	create_alert(&alert);
	gkrellm_alert_config_window(&alert);
	}

static void
create_plugin(GtkWidget *vbox, gint first_create)
	{
	GkrellmStyle     *style;
	GkrellmTextstyle *ts;
	GkrellmPiximage  *krell_image;

	if (first_create)
		panel = gkrellm_panel_new0();

	style = gkrellm_meter_style(style_id);
	ts = gkrellm_meter_textstyle(style_id);
	krell_image = gkrellm_krell_meter_piximage(style_id);
	krell = gkrellm_create_krell(panel, krell_image, style);
	gkrellm_monotonic_krell_values(krell, FALSE);
	gkrellm_set_krell_full_scale(krell, MY_RANGE, 1);

	/* Alerts will appear under text decals but by default will cover up panel
	|  labels.  So here I'll label the panel with a text decal instead of
	|  of a panel label.
	|  However, as of GKrellM 2.1.8 the gkrellm_panel_label_on_top_of_decals()
	|  function provides for panel labels to appear on top of decals so using
	|  a text decal would not be necessary.  See the builtin cpu, proc, or disk
	|  monitors for an example of this.
	*/
	decal = gkrellm_create_decal_text(panel, "Random", ts, style,
					-1, -1, 0);

	gkrellm_panel_configure(panel, NULL, style);

	gkrellm_panel_create(vbox, monitor, panel);

	gkrellm_draw_decal_text(panel, decal, "Random", -1);

	if (first_create)
	    g_signal_connect(G_OBJECT (panel->drawing_area), "expose_event",
    	        G_CALLBACK(panel_expose_event), NULL);
	}

static void
load_config(gchar *arg)
	{
	gchar	config[32], item[CFG_BUFSIZE];

	if (sscanf(arg, "%31s %[^\n]", config, item) != 2)
		return;
	if (!strcmp(config, GKRELLM_ALERTCONFIG_KEYWORD))
		{
		create_alert(&alert);
		gkrellm_load_alertconfig(&alert, item);
		}
	}

static void
save_config(FILE *f)
	{
	gkrellm_save_alertconfig(f, alert, CONFIG_KEYWORD, NULL);
	}

static void
config_plugin(GtkWidget *tab_vbox)
	{
	GtkWidget		*tabs;
	GtkWidget		*vbox;

	tabs = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(tab_vbox), tabs, TRUE, TRUE, 0);

	vbox = gkrellm_gtk_framed_notebook_page(tabs, "Setup");

#if GKRELLM_CHECK_VERSION(2, 1, 8)
	/* Make an alert button containing a nice alert icon.
	*/
	gkrellm_gtk_alert_button(vbox, NULL, FALSE, FALSE, 4, FALSE,
				cb_set_alert, panel);
#else
	gkrellm_gtk_button_connected(vbox, NULL, FALSE, FALSE, 4,
				cb_set_alert, panel, _("Alert"));
#endif
	}

/* The monitor structure tells GKrellM how to call the plugin routines.
*/
static GkrellmMonitor	plugin_mon	=
	{
	CONFIG_NAME,        /* Title for config clist.   */
	0,                  /* Id,  0 if a plugin       */
	create_plugin,      /* The create function      */
	update_plugin,      /* The update function      */
	config_plugin,      /* The config tab create function   */
	NULL,               /* Apply the config function        */

	save_config,        /* Save user config         */
	load_config,        /* Load user config         */
	CONFIG_KEYWORD,     /* config keyword           */

	NULL,               /* Undefined 2	*/
	NULL,               /* Undefined 1	*/
	NULL,               /* private	*/

	MON_MAIL,           /* Insert plugin before this monitor			*/

	NULL,               /* Handle if a plugin, filled in by GKrellM     */
	NULL                /* path if a plugin, filled in by GKrellM       */
	};


  /* All GKrellM plugins must have one global routine named
  |  gkrellm_init_plugin() which returns a pointer to a monitor structure.
  */
GkrellmMonitor *
gkrellm_init_plugin(void)
	{
	pGK = gkrellm_ticks();
	style_id = gkrellm_add_meter_style(&plugin_mon, STYLE_NAME);
	monitor = &plugin_mon;
	return &plugin_mon;
	}
