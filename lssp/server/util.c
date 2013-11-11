
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "header/machdefs.h"
#include "header/rtsp.h"
#include "header/msg_handler.h"
#include "header/util.h"


int debug_toggle = 0;


void
dump_buffer(char *buffer, int n)
{
   int   i;
   int   c = 0;

   printf( "Hex Dump:\n" );   
   for (i = 0; i < n; i++)
   {
      printf ("%02x ", (unsigned char)buffer[i]);
      c += 3;
      if (c > 77)
      {
         printf ("\n");
         c = 0;
      }
   }
   printf ("\n");

   if ( isprint((int) *buffer) || isspace((int) *buffer) )
   {
      printf( "ASCII Dump\n" );
      while ( n-- && (isprint((int) *buffer) || isspace((int) *buffer)) )
         printf( "%c", *buffer++ );
      printf( "\n" );
   }
}

int
parse_url(const char *url, char *server, u_short *port, char *file_name)
{

   int valid_url = 0;
   /* copy url */
   char *full = malloc(strlen(url) + 1);
   strcpy(full, url);
   if(strncmp(full,"rtsp://", 7) == 0)
   {
      char *token;
      int has_port = 0;
      char* aSubStr = malloc(strlen(url) + 1);
      strcpy(aSubStr, &full[7]);
      if (strchr(aSubStr, '/'))
      {
         int len = 0;
		 u_short i = 0;
         char *end_ptr;
         end_ptr = strchr(aSubStr, '/');
         len = end_ptr - aSubStr;
         for (; (i < strlen(url)); i++)
				aSubStr[i] = 0;
         strncpy(aSubStr,&full[7],len);
      }   
      if (strchr(aSubStr, ':'))
         has_port = 1;
      free(aSubStr);

      token = strtok(&full[7], " :/\t\n");
      if(token)
   	{
         strcpy(server, token);
         if(has_port)
         {
            char *port_str = strtok(&full[strlen(server)+7+1], " /\t\n");
            if(port_str)
               *port = (u_short)atol(port_str);
            else
               *port = RTSP_DEFAULT_PORT;
         }
         else
            *port = RTSP_DEFAULT_PORT;
         /* don't requre a file name */
         valid_url = 1;
         token = strtok(NULL, " ");
         if(token)
            strcpy(file_name, token);
         else
            file_name[0] = '\0';
      }
   }
   else
   {
      /* try just to extract a file name */
      char *token = strtok(full, " \t\n");
      if(token)
      {
         strcpy(file_name, token);
         server[0] = '\0';
         valid_url = 1;
      }
   }
   free(full);
   return valid_url;
}

void
free_streams( struct SESSION_STATE *state )
{
   STREAM   *s;         /* stream descriptor pointer */
   STREAM   *n;
   
   for ( s = state->streams; s; s = n )
   {
      n = s->next;
      if ( s->stream_fd )
         close( s->stream_fd );
      free( s );
   }
   state->streams = 0;
}

int big_endian()
{
	short test = 0x00ff;
	char* p = (char*)&test;
	return (p[0] == 0);
}

void swap_word(u_int16* pWord, int nWords)
{
	int i;
	u_int16 tmp;
	char* pTmp = (char*)&tmp;

	for(i=0;i<nWords;++i)
	{
		char* pData = (char*)&pWord[i];

		pTmp[0] = pData[1];
		pTmp[1] = pData[0];
		pWord[i] = tmp;
	}
}

void swap_dword(u_int32* pDWord, int nDWords)
{
	int i;
	u_int32 tmp;
	char* pTmp = (char*)&tmp;

	for(i=0;i<nDWords;++i)
	{
		char* pData = (char*)&pDWord[i];

		pTmp[0] = pData[3];
		pTmp[1] = pData[2];
		pTmp[2] = pData[1];
		pTmp[3] = pData[0];
		pDWord[i] = tmp;
	}
}
