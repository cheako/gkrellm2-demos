/* demo4.c  - requires GKrellM 2.0.0 or better
|
|     make
|     gkrellm -p demo4.so
|
|  To check the themeing features for plugins, run from this directory:
|		gkrellm -p demo4.so -theme test-theme
|  And then go to the theme configuration and select alternate themes 0 and 1.
|
|  This is a more advanced GKrellM plugin demo which emaphasizes the theming
|  considerations that might go into a full featured plugin.  It is not always
|  desirable to maximize themability because sometimes it makes sense to fix
|  object positions in a panel layout.  But the point of this demo is to
|  illustrate theming so the choice here is to make all positioning
|  configurable in the gkrellmrc.
|
|  This is a meter styled panel that has:
|    * Two krells with #included images and gkrellmrc extension styles.
|    * A krell implemented as a slider using the builtin slider image and
|      style.  The slider is themed using an extension style.
|    * Two overlapping decal buttons that are not rectangular in shape and so
|      have a custom in button callback.  The button images, sizes, and
|      positions are completely themable.
|
|  There is also a configuration tab and prototype save and load user config
|  functions.
|
|  WIN32 defines in this demo are for portability to Windows.
*/


#if !defined(WIN32)
#include <gkrellm2/gkrellm.h>
#else
#include <src/gkrellm.h>
#include <src/win32-plugin.h>
#endif

#include "AB_krell.xpm"
#include "center_button.xpm"
#include "rim_button.xpm"


  /* CONFIG_NAME is the name that will appear in the Config window list
  |  STYLE_NAME is the name used for subdirectory image searching and for
  |  gkrellmrc Style line settings for the plugin.
  */
#define	CONFIG_NAME	"Demo4"
#define	STYLE_NAME	"demo4"


typedef struct
	{
	GkrellmPiximage     *image;
	GkrellmDecalbutton  *button;
	gint                x, y, w, h;
	double              x_scale,
                        y_scale;
	}
	ScaledButton;

static ScaledButton center_button,
                    rim_button;

static GkrellmMonitor *monitor;
static GkrellmPanel *panel;
static GkrellmTicks *pGK;

static GkrellmKrell *slider_krell,
                    *slider_in_motion;
static double       slider_value;

static GkrellmKrell *A_krell,
                    *B_krell;

static gint         plug_var1;
static gint         style_id;   /* A plugin specific id set in init_plugin() */

static gboolean		resources_acquired_flag;


static gint
panel_expose_event(GtkWidget *widget, GdkEventExpose *ev, GkrellmPanel *p)
    {
	gdk_draw_pixmap(widget->window,
			widget->style->fg_gc[GTK_WIDGET_STATE (widget)], p->pixmap,
			ev->area.x, ev->area.y, ev->area.x, ev->area.y,
			ev->area.width, ev->area.height);
    return TRUE;
    }


  /* Just a demo here, make the krell slide across the panel in 15 seconds.
  */
static void
update_plugin()
	{
	static gint	t;

	/* This routine is called at the update rate which is 2 - 10
	|  times per second.
	*/
	if (!pGK->second_tick)
		return;
	++t;
	gkrellm_update_krell(panel, A_krell, t % 15);
	gkrellm_update_krell(panel, B_krell, 15 - 2 * (t % 8));
	gkrellm_draw_panel_layers(panel);
	}

static void
update_slider_position(GkrellmKrell *k, gint x_ev)
	{
	gint	x;

	/* Consider only x_ev values that are inside the dynamic range of the
	|  krell, ie from k->x0 to k->x0 + k->w_scale.  The krell left and right
	|  margins determined this range when the slider krell was created.
	|  I also set the krell full_scale to be w_scale so I can simply update
	|  the krell position to the x position within w_scale.
	*/
	x = x_ev - k->x0;
	if (x < 0)
		x = 0;
	if (x > k->w_scale)
		x = k->w_scale;
	gkrellm_update_krell(panel, k, x);
	gkrellm_draw_panel_layers(panel);

	/* Do whatever needs to be done when the slider x position changes.
	*/
	slider_value = (double) x / (double) k->w_scale;  /* ranges 0.0 - 1.0 */
	}

  /* To make a krell work like a slider, we need to update the krell position
  |  as a function of mouse position.
  */
static gint
cb_panel_motion(GtkWidget *widget, GdkEventMotion *ev, gpointer data)
	{
	GdkModifierType state;

	if (slider_in_motion)
		{
		/* Check if button is still pressed, in case missed button_release
		*/
		state = ev->state;
		if (!(state & GDK_BUTTON1_MASK))
			slider_in_motion = NULL;
		else
			update_slider_position(slider_in_motion, (gint) ev->x);
		}
	return TRUE;
	}

static gint
cb_panel_release(GtkWidget *widget, GdkEventButton *ev, gpointer data)
	{
	if (slider_in_motion)
		printf("Slider value is %.2f\n", slider_value);
	slider_in_motion = NULL;
	return TRUE;
	}

static gint
cb_panel_press(GtkWidget *widget, GdkEventButton *ev, gpointer data)
	{
	GkrellmKrell *k;

	if (ev->button == 3)
		{
		gkrellm_open_config_window(monitor);
		return TRUE;
		}
    /* Check if button was pressed within the space taken
    |  up by the slider krell.
    */
	slider_in_motion = NULL;
	k = slider_krell;
	if (   ev->x > k->x0 && ev->x <= k->x0 + k->w_scale
	    && ev->y >= k->y0 && ev->y <= k->y0 + k->h_frame
	   )
		slider_in_motion = k;

	if (slider_in_motion)
		update_slider_position(slider_in_motion, (gint) ev->x);
	return TRUE;
	}

static void
cb_button_clicked(GkrellmDecalbutton *button, gpointer data)
	{
	printf("button %d was pressed.\n", GPOINTER_TO_INT(data));
	}


  /* If the mouse is in the button decal and the mouse position is not on a
  |  transparent pixel (look at the pixbuf), return TRUE.
  */
static gboolean
cb_in_button(GkrellmDecalbutton *b, GdkEventButton *ev, ScaledButton *sbut)
	{
	GdkPixbuf       *pixbuf;
	GkrellmDecal    *d;
	guchar          *pixels, alpha;
	gint            rowstride, x, y;

	d = b->decal;
	if (gkrellm_in_decal(d, ev))
		{
		pixbuf = sbut->image->pixbuf;
		if (!gdk_pixbuf_get_has_alpha(pixbuf))
			return TRUE;
		x = ev->x - d->x;
		x = x / sbut->x_scale;
		y = ev->y - d->y;
		y = y / sbut->y_scale;
		pixels = gdk_pixbuf_get_pixels(pixbuf);
		rowstride = gdk_pixbuf_get_rowstride(pixbuf);
		pixels += y * rowstride + 4 * x;
		alpha = *(pixels + 3);
		if (alpha > 0)
			return TRUE;
		}
	return FALSE;
	}

static gint
reference_x_position(gint x, gchar *ref)
	{
	/* Re-reference the x position to panel center or the right panel edge
	*/
	if (ref[0] == 'c' || ref[0] == 'C')
		x += gkrellm_chart_width() / 2;
	else if (ref[0] == 'r' || ref[0] == 'R')
		x = gkrellm_chart_width() - x - 1;
	return x;
	}

  /* Make a scaled button that has a fully themable image, size and position.
  |  Handle possibly irregulary shaped buttons with transparency by using
  |  a in button callback that checks if the mouse is on a transparent pixel.
  */
static void
make_button(ScaledButton *sbut, gchar *name, gchar **name_xpm, gchar *position,
		gint x, gint y, gint fn_id)
	{
	gint	w = 0, h = 0;
	gchar	*s, x_ref[8];

	sbut->x = x;
	sbut->y = y;
	gkrellm_load_piximage(name, name_xpm, &sbut->image, STYLE_NAME);
	sbut->w = gdk_pixbuf_get_width(sbut->image->pixbuf);
	sbut->h = gdk_pixbuf_get_height(sbut->image->pixbuf) / 2;
	sbut->x_scale = sbut->y_scale = 1.0;

	x_ref[0] = 'l';		/* Default x position reference to left panel edge */
	if ((s = gkrellm_get_gkrellmrc_string(position)) != NULL)
		{
		sscanf(s, "%d %d %d %d %8s", &sbut->x, &sbut->y, &w, &h, x_ref);
		sbut->x = reference_x_position(sbut->x, x_ref);
		if (w > 0)
			{
			sbut->x_scale = (double) w / (double) sbut->w;
			sbut->w = w;
			}
		if (h > 0)
			{
			sbut->y_scale = (double) h / (double) sbut->h;
			sbut->h = h;
			}
		}
	sbut->button = gkrellm_make_scaled_button(panel, sbut->image,
			cb_button_clicked, GINT_TO_POINTER(fn_id), FALSE, FALSE, 2, 0, 1,
			sbut->x, sbut->y, sbut->w, sbut->h);
	gkrellm_set_in_button_callback(sbut->button, cb_in_button, sbut);
	}

	/* If the plugin is enabled and uses some resources, it may need to do
	|  some cleanup if it is later disabled.  For this case you need to
	|  connect to the disable.  The test to acquire the resources should be
	|  made in the create_plugin() function because it will be called when
	|  the plugin is enabled.
	*/
static void
cb_plugin_disabled(void)
	{
	printf("Demo4 plugin is being disabled.\n");
	/* free resources */
	resources_acquired_flag = FALSE;
	}

static void
create_plugin(GtkWidget *vbox, gint first_create)
	{
	GkrellmStyle    *plugin_style, *krell_style;
	GkrellmMargin	*m;
	GkrellmKrell	*k;
	GkrellmPiximage *krell_image = NULL;

	if (first_create)
		panel = gkrellm_panel_new0();

	if (!resources_acquired_flag)
		{
		/* Acquire any needed resources */
		resources_acquired_flag = TRUE;
		gkrellm_disable_plugin_connect(monitor, cb_plugin_disabled);
		}

	plugin_style = gkrellm_meter_style(style_id);
	m = gkrellm_get_style_margins(plugin_style);

	/*  ==== Create two independantly themable krells with custom images ====
	|  One way to do this would be to use the default single krell that all
	|  panels have for the first krell (call it A_krell) and create a custom
	|  extension krell for the second B_krell.  This would lead to possible
	|  gkrellmrc lines like:
	|     StyleMeter   demo4.krell_yoff = 0
	|     StyleMeter   demo4.B_krell.krell_yoff = 8
	|  These style lines are inconsistent and this approach will also lead
	|  to non uniform coding of the two krells because we are not using the
	|  default meter area krell image in this demo.  So a second better way
	|  would be to make both krells custom extension krells and just not use
	|  the default panel krell.  This yields cleaner coding and would lead to
	|  consistent possible gkrellmrc lines like:
	|     StyleMeter   demo4.A_krell.krell_yoff = 0
	|     StyleMeter   demo4.B_krell.krell_yoff = 8
	*/
	/* Make the A_krell and the B_krell with the same #included image.  But
	|  set it up so a theme may have different A_krell.png or B_krell.png
	|  images for them.  The gkrellm_meter_style_by_name() call provides for
	|  the gkrellmrc StyleMeter lines. gkrellm_set_style_krell_values_default()
	|  will set default krell values only for those parameters that have not
	|  had custom StyleMeter lines in the gkrellmrc.  Ie, whatever the themer
	|  sets in the gkrellmrc overrides our defaults.
	|  I'll set the A_krell default position to be at the panel top margin and
	|  the default B_krell position to be justified to the bottom margin.
	*/
	/* WARNING: krell_image must be static or initialized to NULL */
	gkrellm_load_piximage("A_krell", AB_krell_xpm, &krell_image, STYLE_NAME);
	krell_style = gkrellm_copy_style(
				gkrellm_meter_style_by_name("demo4.A_krell"));
	gkrellm_set_style_krell_values_default(krell_style,
					-1,                 /* -1 justifies to top margin */
					1, 3,               /* depth, x_hot */
					KRELL_EXPAND_NONE,  /* See the Themes doc! */
					1,                  /* ema period, almost always 1 */
					24, 0);             /* left & right margins */
	A_krell = gkrellm_create_krell(panel, krell_image, krell_style);
	gkrellm_monotonic_krell_values(A_krell, FALSE);
	gkrellm_set_krell_full_scale(A_krell, 15, 1);
	gkrellm_update_krell(panel, A_krell, 0);
	g_free(krell_style);

	gkrellm_load_piximage("B_krell", AB_krell_xpm, &krell_image, STYLE_NAME);
	krell_style = gkrellm_copy_style(
				gkrellm_meter_style_by_name("demo4.B_krell"));
	gkrellm_set_style_krell_values_default(krell_style,
					-3,	                /* -3 justifies to bottom margin. */
					1, 3,
					KRELL_EXPAND_NONE, 1, 24, 0);
	B_krell = gkrellm_create_krell(panel, krell_image, krell_style);
	gkrellm_monotonic_krell_values(B_krell, FALSE);
	gkrellm_set_krell_full_scale(B_krell, 15, 1);
	gkrellm_update_krell(panel, B_krell, 0);
	g_free(krell_style);


	/* ==== Create a krell slider using the builtin slider ====
	|  There is a builtin krell slider image and style we can use, or krells
	|  used as sliders can be created exactly as above and that would allow
	|  for slider images to be independently themed.  It will probably be a
	|  consideration of possible slider interaction with other objects in
	|  a panel or how the slider is used that determines which method should
	|  be used.  For example, the GKrellMSS plugin uses an extension krell
	|  image and style for its slider because the slider is animated and
	|  there are other krells which move in the same space as the slider.
	|  It would be unlikely that the general purpose builtin slider image would
	|  work well in this situation for all themes.
	|  However, consistent slider appearance for a theme really is desirable if
	|  possible and for plugins where slider behavior is more deterministic,
	|  using the builtin slider image is best.  But it is important to
	|  recognize that some of the builtin slider style values will be common to
	|  all sliders while some can be set per slider.  The gkrellmrc provides
	|  for setting the common image related values (depth, x_hot, and expand),
	|  but a plugin can set up the per slider positioning values (yoff,
	|  left margin and right margin) by defining an extension style so there
	|  can be gkrellmrc lines like:
	|      StyleMeter  demo4.slider.krell_yoff = 0
	|      StyleMeter  demo4.slider.krell_left_margin = 2
	|      StyleMeter  demo4.slider.krell_right_margin = 2
	|
	|  The gkrellm_set_style_slider_values_default() function used below
	|  sets only the positioning parameters of the builtin slider style.  It
	|  sets them to any positioning values that were set in the gkrellmrc or
	|  to our default values if the corresponding parameter was not set in the
	|  gkrellmrc.
	|  Here I'll set the default slider position just below the A_krell and
	|  set the slider default left margin to the right of the buttons.
	|  If I set the krell full scale value to match the physical range of
	|  motion of the krell, it will be easy to update the slider position.
	*/
	krell_image = gkrellm_krell_slider_piximage();
	krell_style = gkrellm_copy_style(
				gkrellm_meter_style_by_name("demo4.slider"));
	gkrellm_set_style_slider_values_default(krell_style,
				A_krell->y0 + A_krell->h_frame + 2,		/* yoff */
				24, 0);			/* Left and right krell margins */
	k = gkrellm_create_krell(panel, krell_image, krell_style);
	/* I would free krell_style here, except it will be used below where I
	|  demonstrate krell positioning after the panel is created.
	*/
	gkrellm_monotonic_krell_values(k, FALSE);
	gkrellm_set_krell_full_scale(k, k->w_scale, 1 /* almost always 1 */);
	gkrellm_update_krell(panel, k, 0);
	slider_krell = k;


	/* ==== Make a couple of shaped scaled buttons ====
	*/
	make_button(&rim_button, "rim_button", rim_button_xpm,
			"demo4_rim_button_position", 2, 0, 0);
	make_button(&center_button, "center_button", center_button_xpm,
			"demo4_center_button_position", 7, 5, 1);


	/* All buttons and krells are created, so configure the panel.
	|  The result of the configure is that a panel height
	|  is calculated to accomodate all the objects on the panel.
	*/
	gkrellm_panel_configure(panel, NULL /*no label*/, plugin_style);
	gkrellm_panel_create(vbox, monitor, panel);
	gkrellm_draw_panel_layers(panel);

	/* The initial placement of objects can be adjusted after they are created.
	|  For example, if we didn't want the slider placed below the A_krell and
	|  instead want it to be centered in the panel, we can do this now after
	|  the panel is created and we know its height.  But do it only if the
	|  theme is not setting the slider y offset.
	*/
	if (!gkrellm_style_is_themed(krell_style, GKRELLMSTYLE_KRELL_YOFF))
		gkrellm_move_krell_yoff(panel, slider_krell,
				(panel->h - slider_krell->h_frame) / 2);
	g_free(krell_style);

	/* Final cleanup, connect signals for expose and mouse events.
	|  You need to handle your own mouse events and handle your sliders,
	|  but the buttons are handled by gkrellm (callback was assigned above).
	*/
	if (first_create)
		{
		g_signal_connect(G_OBJECT(panel->drawing_area), "expose_event",
				G_CALLBACK(panel_expose_event), panel);
		g_signal_connect(G_OBJECT(panel->drawing_area), "button_press_event",
				G_CALLBACK(cb_panel_press), NULL );
		g_signal_connect(G_OBJECT(panel->drawing_area), "button_release_event",
				G_CALLBACK(cb_panel_release), NULL );
		g_signal_connect(G_OBJECT(panel->drawing_area), "motion_notify_event",
				G_CALLBACK(cb_panel_motion), NULL);
		}
	}



/* --------------------------------------------------------------------	*/
/* Configuration														*/

static GtkWidget	*plug_var1_button;

/* Select a plugin config keyword.  It needs to be different from any
|  builtin or other plugin keyword.  It's best to make it the same as
|  your style name.
*/
#define PLUGIN_CONFIG_KEYWORD	STYLE_NAME


  /* As an example save and load the state of plug_var1
  */
static void
save_plugin_config(FILE *f)
	{
	fprintf(f, "%s type1 %d\n", PLUGIN_CONFIG_KEYWORD, plug_var1);
	}

static void
load_plugin_config(gchar *arg)
	{
	gchar	config[64], item[256];
	gint	n;

	n = sscanf(arg, "%s %[^\n]", config, item);
	if (n == 2)
		{
		if (strcmp(config, "type1") == 0)
			sscanf(item, "%d\n", &plug_var1);
		}
	}

static void
apply_plugin_config()
	{
	gint	new_var1;

	new_var1 = GTK_TOGGLE_BUTTON(plug_var1_button)->active;
	if (new_var1 != plug_var1)
		{
		/* Do some configuration */
		}
	plug_var1 = new_var1;
	}

static gchar	*plugin_info_text[] =
{
"<h>Example plugin config\n",
"\n\tPut any documentation here to explain your plugin.\n",
"\tText can be ",
"<b>bold",
" or ",
"<i>italic",
" or ",
"<ul>underlined.\n",
};


static void
create_plugin_tab(GtkWidget *tab_vbox)
	{
	GtkWidget		*tabs;
	GtkWidget		*vbox;
	GtkWidget		*text;
	gint			i;

	/* Make a couple of tabs.  One for setup and one for info
	*/
	tabs = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(tabs), GTK_POS_TOP);
	gtk_box_pack_start(GTK_BOX(tab_vbox), tabs, TRUE, TRUE, 0);

/* --Setup tab */
	vbox = gkrellm_gtk_framed_notebook_page(tabs, "Setup");

	/* Replace this single button with your configuration widgets */
	gkrellm_gtk_check_button(vbox, &plug_var1_button, plug_var1,
			FALSE, 0, "Set plug_var1");

/* --Info tab */
	vbox = gkrellm_gtk_framed_notebook_page(tabs, "Info");
	text = gkrellm_gtk_scrolled_text_view(vbox, NULL,
				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	for (i = 0; i < sizeof(plugin_info_text)/sizeof(gchar *); ++i)
		gkrellm_gtk_text_view_append(text, plugin_info_text[i]);
 	}

/* The monitor structure tells GKrellM how to call the plugin routines.
*/
static GkrellmMonitor	plugin_mon	=
	{
	CONFIG_NAME,        /* Name, for config tab.    */
	0,                  /* Id,  0 if a plugin       */
	create_plugin,      /* The create function      */
	update_plugin,      /* The update function      */
	create_plugin_tab,  /* The config tab create function   */
	apply_plugin_config, /* Apply the config function        */

	save_plugin_config, /* Save user config   */
	load_plugin_config, /* Load user config   */
	PLUGIN_CONFIG_KEYWORD, /* config keyword  */

	NULL,               /* Undefined 2  */
	NULL,               /* Undefined 1  */
	NULL,               /* Undefined 0  */

	MON_UPTIME,         /* Insert plugin before this monitor.  Choose   */
	                    /*   MON_CLOCK, MON_CPU, MON_PROC, MON_DISK,    */
	                    /*   MON_INET, MON_NET, MON_FS, MON_MAIL,       */
	                    /*   MON_APM, or MON_UPTIME                     */

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
	/* Here is a good place to set any config defaults for the plugin.
	|  This is called before config file parsing calls your
	|  load_plugin_config()
	|  But don't do anything that assumes your plugin will actually
	|  be running because it may not be enabled.
	*/
	plug_var1 = TRUE;
	pGK = gkrellm_ticks();

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
