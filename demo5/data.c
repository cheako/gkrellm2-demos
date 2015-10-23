/* demo5d - requires GKrellM 2.2.0 or better
|
|	Data collection functions which will be common to both the client
|	and server plugins.  There should be no gkrellm, gkrellmd, or Gtk
|	function calls here.  There may be glib calls, but at least try
|	to use calls compatible with glib1.2 in case the server plugin is
|	compiled on a machine with only glib 1.2.  If glib calls,
|   #include <glib.h>
*/
#include <stdio.h>
#include <time.h>

#include "demo5-common.h"


  /* Just count the time since gkrellm or gkrellmd start.
  |  Let the krell be sort of a second hand and a text label string
  |  indicate the number of minutes.
  */
void
demo5_data_read(Demo5Data *data)
	{
	static time_t	t_zero;
	int				t;
	static char		buf[32];

	if (t_zero == 0)
		t_zero = time(NULL);

	t = (int) (time(NULL) - t_zero);
	data->value = t % 60;
	t /= 60;
	snprintf(buf, sizeof(buf), "%d min", t);
	data->label = buf;
	}

