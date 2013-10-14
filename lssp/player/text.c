
#include <sys/soundcard.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include "machdefs.h"
#include "socket.h"
#include "player.h"
#include "../server/session.h"
#include <string.h>
#include <stdlib.h>
/*  nasty globals  */
static int wav_session = 0;
static int wav_fd = 0;
static int wav_rate = 0;
static int wav_sampleformat = 0;

int errno = 0;
/*  end of nasty globals */


void interface_init()
{
}

void interface_can_play(int id)
{
    printf("Can play %d\n", id);
}

void interface_close_stream(int id)
{
    printf( "CLOSE\n" );
}

void interface_new_session(int id)
{
    printf("New session: %d\n", id);
}

void interface_close_session(int id)
{
    printf("Close session: %d\n", id);
}

void interface_hello(const char *filename)
{
    printf ("HELLO\n");
	if (strlen(filename)) {
		char cmdbuf[20];
        sprintf(cmdbuf, "g %s", filename);
        handle_command(cmdbuf);
	}
}

void interface_renew_play( u_long id )
{
   printf( "Enter new command\n" );
}

void interface_start_play( u_long id )
{
   printf( "PLAY started.\n" );
}

void
convert16to8(u_short n, char *data)
{
  int i;

  for (i=0;i<=(n/2);i++) {
    data[i]=data[i*2]-128;
  }
}

void
interface_stream_output(u_long id, u_short n, char *data)
{
    if (wav_session == id) {
    	if (!wav_fd)
    	{
    	    int x;

    	    wav_fd = open("/dev/dsp", O_WRONLY);
    	    wav_rate = PCM_hdr.nSamplesPerSec;
            if(PCM_hdr.wBitsPerSample==16) {
              x = AFMT_S16_BE;
	      if(ioctl(wav_fd, SNDCTL_DSP_SETFMT, &x)==-1)
              {
                 printf("Error: Cannot set sample format\n");
		 exit(1);
              }
	      wav_sampleformat = x; 
              printf("Sample Format (supported by card): %d\n", x);
	    }
	    else if (PCM_hdr.wFormatTag==0x7)
	    {
	      /*  muLaw  */
	      x = AFMT_MU_LAW;
	      if(ioctl(wav_fd, SNDCTL_DSP_SETFMT, &x)==-1)
              {
                 printf("Error: Cannot set sample format\n");
		 exit(1);
              }
	      wav_sampleformat = x; 
	    }
	    else
	    {
	      /*  assume unsigned 8-bit, and that /dev/dsp is all setup */
	      wav_sampleformat = AFMT_U8;
	    }
	      
    	    x = PCM_hdr.nChannels -1;	
	    if (ioctl(wav_fd, SNDCTL_DSP_STEREO, &x)==-1) {
                 printf("Error: Cannot set channels\n");
		 exit(1);
	    }
    	    x = wav_rate;		
	    if (ioctl(wav_fd, SNDCTL_DSP_SPEED, &x)==-1) {
                 printf("Error: Cannot set speed\n");
	    }

	    printf("WAV File\n");
	    printf("Frequency: %ld\n", PCM_hdr.nSamplesPerSec);
	    printf("Channels: %d\n", PCM_hdr.nChannels);
            printf("Bits Per Sample: %d\n", PCM_hdr.wBitsPerSample);
    	}
	
	if(PCM_hdr.wBitsPerSample==16 && wav_sampleformat == AFMT_U8 ) 
        {
	  convert16to8(n, data);
	  write (wav_fd, data, (n/2));
	}
	else
        {
	  write (wav_fd, data, n);
	}
    	return;
    }
}

void
interface_stream_done( u_long sessionId )
{
    printf( "PLAY done.\n" );
    close (wav_fd);
    wav_fd = 0;
    wav_session = 0;
}

void
interface_error( const char *err )
{
    printf( err );
}

void
interface_alert_msg( const char *msg )
{
    printf( msg );
}
