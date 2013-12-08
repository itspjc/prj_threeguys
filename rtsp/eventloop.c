#include <string.h>
#include "parser.h"

int rtsp_sock = 0;
int file_register = 0;
FILE* file;
FILE* inputStream = NULL;

void eventloop(){

	if(!file_register){
		//inputStream = file;			
		file_register = 1;
	}
	parse_rtsp();
}
