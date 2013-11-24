#include <string.h>
#include "parser.h"

int rtsp_sock = 0;
int file_register = 0;
FILE* inputStream = NULL;

void eventloop(FILE *file){

	if(!file_register){
		inputStream = file;			
		file_register = 1;
	}
	parse_rtsp();
}
