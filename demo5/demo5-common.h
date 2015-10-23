
/* Header common to demo5.c, demo5d.c and data.c
*/

#define DEMO5_VERSION_MAJOR		1
#define DEMO5_VERSION_MINOR		0
#define DEMO5_VERSION_REV		0


  /* The style name will be the theme subdirectory for custom images for this
  |  plugin and it will be the gkrellmrc style name for custom settings.
  |  It must be unique with respect to other monitors so might as well also
  |  be used as the gkrellmd serve name.  The serve name is used
  |  to tag data sent over the network as belonging to this plugin and both
  |  the client and server plugin modules must agree on its value.
  */
#define	DEMO5_STYLE_NAME		"demo5"
#define DEMO5_SERVE_NAME		DEMO5_STYLE_NAME

typedef struct
	{
	int			value;
	char		*label;
	int			value_changed,
				label_changed;
	}
	Demo5Data;


  /* The actual data collection function.  The server plugin will call it
  |  directly.  The client plugin will have a function pointer which may
  |  be set to call it depending on client mode and whether there is a
  |  server plugin loaded.
  */
void	demo5_data_read(Demo5Data *data);
