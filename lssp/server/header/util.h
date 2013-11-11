
#ifndef _UTIL_H_
#define _UTIL_H_

#include "header/machdefs.h"
#include "header/session.h"
#include "header/socket.h"

void dump_buffer(char *, int);
int parse_url(const char *url, char *server, u_short *port, char *file_name);
void free_streams( struct SESSION_STATE *state );
void swap_word(u_int16* pWord, int nWords);
void swap_dword(u_int32* pDWord, int nDWords);
int big_endian();


#endif	/* _UTIL_H_ */
