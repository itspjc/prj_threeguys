#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(void)
{
	FILE* file = fopen("sample.ts", "rb");
	if(file == NULL){
		printf("?\n");
	}
	int len;
	char buf[255];
    char buf2[255];
    int i;

    len = fread(buf, 188, 1, file);
        for(i = 0; i < 188; i +=4){
                char d[4], e[4];
                d[0] = buf[i];
                d[1] = buf[i+1];
                d[2] = buf[i+2];
                d[3] = buf[i+3];

				e[0] = d[3];
				e[1] = d[2];
				e[2] = d[1];
				e[3] = d[0];
                memcpy(&buf2[i], e, 4);
           }
    for(i =0; i < 16; i++){
        printf("%x ", buf[i]);
                }
             printf("\n");
    for(i =0; i < 16; i++){
        printf("%x ", buf2[i]&0xFF);
                }
	
	
	if(len < 0){
		printf("eeee:?\n");
	}	
}	
