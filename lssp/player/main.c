
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "machdefs.h"
#include "eventloop.h"
#include "socket.h"
#include "rtsp.h"
#include "util.h"
#include "player.h"
#include "scheduler.h"
extern void interface_init();

void
playercmd(RTSP_SOCK fd)
{
     char buffer[256];
     handle_command(fgets(buffer, sizeof(buffer), stdin));
}

int
main (int argc, char **argv)
{
   eventloop_init();
   init_player();
   readers[1] = playercmd;
   interface_init(0);

   if (argc > 1) {
     char cmdbuf[100];
     sprintf(cmdbuf, "o %s", argv[1]);
     handle_command(cmdbuf);
   }

   puts( "command : " );
   while (1)
      eventloop();
   return 0;
}
