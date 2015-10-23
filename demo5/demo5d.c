/* demo5d.c	- requires GKrellM 2.2.0 or better
|
|	This is the gkrellmd server plugin part of demo5.  It reads data
|   from the data reading module of this plugin in data.c and transmits
|   the data to client gkrellms.
|
|   A server plugin always reads data from the common data reading module.
|
|   A client mode capable gkrellm plugin will read data using the same
|   common data reading module if gkrellm is not run in client mode, but
|   will read its data from separate client mode code if gkrellm is run
|   in client mode.  The client mode code will connect to the gkrellm
|   client interface so it can receive the network transmitted data lines
|   that this gkrellmd server plugin will send.
*/


#include <gkrellm2/gkrellmd.h>

#include "demo5-common.h"



static Demo5Data	demo5d_data;

  /* Do whatever data collection is needed at whatever interval when
  |  the update function is called.  "first_update" is a flag which
  |  is TRUE for the first update call and should be used to ensure that
  |  valid data will be available for client initial updates when gkrellmd
  |  first starts up.  If it's possible for data to be unchanged when it
  |  is read, then a plugin should check that and request that its data
  |  be served only if there is a change (to conserve network bandwidth).
  */
static void
demo5d_update(GkrellmdMonitor *mon, gboolean first_update)
	{
	Demo5Data	tmp_data;

	if (!GK.five_second_tick && !first_update)
		return;

	demo5_data_read(&tmp_data);

	/* gkrellmd_dup_string(gchar **dst, gchar *src) is a useful function that
	|  returns TRUE if strings were not equal and the src was duped into the
	|  dst.
	*/
	demo5d_data.label_changed
				= gkrellmd_dup_string(&demo5d_data.label, tmp_data.label);
	demo5d_data.value_changed = (tmp_data.value != demo5d_data.value);
	demo5d_data.value = tmp_data.value;

	/* There will be an automatic serve after updates for which first_update
	|  is TRUE.  There will otherwise be a data serve for this monitor only
	|  if requested, so request it if data values have changed.
	*/
	if (demo5d_data.label_changed || demo5d_data.value_changed)
		gkrellmd_need_serve(mon);
	}


  /* This function is called to collect data to be sent to clients.
  |  It will be called for each client that data is to be sent to and the
  |  "first_serve" flag will be true for the first data serve to each
  |  client.  Use the first_serve flag to know if you should transmit
  |  all avaialable data or if you can transmit only changed data.
  */
static void
demo5d_serve_data(GkrellmdMonitor *mon, gboolean first_serve)
	{
	gchar	buf[32];

	/* Must set our serve name and its value must agree with
	|  the name the gkrellm client demo5 plugin sets up to receive.
	|  If warranted, you can transmit unrelated data groups under different
	|  serve names as long as the client plugin is connected to receive them.
	|  Serve names better be unique with respect to other monitors, so use
	|  a name derived from your plugin namespace and probably just use
	|  the same name used for your STYLE_NAME.
	*/
	gkrellmd_set_serve_name(mon, DEMO5_SERVE_NAME);

	if (demo5d_data.value_changed || first_serve)
		{
		snprintf(buf, sizeof(buf), "value %d\n", demo5d_data.value);
		gkrellmd_serve_data(mon, buf);
		}

	if (demo5d_data.label_changed || first_serve)
		{
		snprintf(buf, sizeof(buf), "label %s\n", demo5d_data.label);
		gkrellmd_serve_data(mon, buf);
		}
	}

  /* The callback for reading data sent from clients.  The data should
  |  be line structured ascii.  Lines received here will be '\n' terminated
  |  even if the client did not append a '\n'.  The possibility that
  |  multiple clients are connected must be considered here.
  */
#if defined(GKRELLMD_CHECK_VERSION)
#if GKRELLMD_CHECK_VERSION(2,2,5)
static void
demo5d_client_input_read(GkrellmdClient *client, gchar *line)
	{
	printf("Received from %s: %s", client->hostname, line);
	}
#endif
#endif

static void
demo5d_serve_setup(GkrellmdMonitor *mon)
	{
	gchar	buf[64];

	/* An "available" setup line is automatically generated for client mode
	|  plugins if gkrellmd enables their server plugin counterpart.
	|  Otherwise, send whatever initial parameters that might be
	|  needed.  For this example, just send our plugin version numbers.
	*/
	snprintf(buf, sizeof(buf), "version %d %d %d",
			DEMO5_VERSION_MAJOR,
			DEMO5_VERSION_MINOR,
			DEMO5_VERSION_REV);

    gkrellmd_plugin_serve_setup(mon, DEMO5_SERVE_NAME, buf);
//    gkrellmd_plugin_serve_setup(mon, DEMO5_SERVE_NAME, "some other setup");

	/* If a server plugin expects clients to be sending data, here's where
	|  you should register your function to read input from clients.  This
	|  is likely not needed for most plugins.
	*/
#if defined(GKRELLMD_CHECK_VERSION)
#if GKRELLMD_CHECK_VERSION(2,2,5)
	gkrellmd_client_input_connect(mon, demo5d_client_input_read);
#endif
#endif
	}


  /* Set up a monitor structure and return it from gkrellmd_init_plugin()
  |  just as a normal gkrellm plugin does.
  */
static GkrellmdMonitor	demo5d_monitor =
	{
	DEMO5_SERVE_NAME,
	demo5d_update,
	demo5d_serve_data,
	demo5d_serve_setup,
	};

  /* The one required global function a gkrellmd server plugin must have.
  |  It must be named gkrellmd_init_plugin()
  */
GkrellmdMonitor *
gkrellmd_init_plugin(void)
	{
#if defined(GKRELLMD_CHECK_VERSION)
#if GKRELLMD_CHECK_VERSION(2,2,5)
	const gchar	*config;
#endif
#endif

	/* Do any convenient data initializations and return a pointer to our
	|  monitor structure to indicate it's OK to enable this plugin.
	|  Once a gkrellmd plugin is enabled, it will not be disabled and will
	|  transmit data to all connected clients.
	*/
	demo5d_data.label = g_strdup("");

	/* Gkrellmd plugins can have configuration blocks in the gkrellmd.conf
	|  gkrellmd server configuration file.  The blocks must be bracketed with
	|  lines [plugin-name] ... [/plugin-name] where plugin-name is the name
	|  given in the GkrellmdMonitor structure.  For our demo5 example where
	|  DEMO5_SERVE_NAME is "demo5", we can have a gkrellmd.conf block like:
	|
	|  [demo5]
	|  config line 1
	|  config line 2
	|  [/demo5]
	|
	|  and we can read those config lines like so:
	*/
#if defined(GKRELLMD_CHECK_VERSION)
#if GKRELLMD_CHECK_VERSION(2,2,5)
	while ((config = gkrellmd_config_getline(&demo5d_monitor)) != NULL)
		printf("demo5d gkrellmd.conf line: \"%s\"\n", config);
#endif
#endif

	return &demo5d_monitor;
	}
