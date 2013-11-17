#ifndef __TS__H__
#define __TS__H__

#define FILE_NAME "~/Now You See Me 2013 EXTENDED 1080p BRRip x264 AC3-JYK.ts"

#define TRANSPORT_PACKET_SIZE 188

#define TRANSPORT_SYNC_BYTE 0x47

char buffer[TRANSPORT_PACKET_SIZE];
uint16_t pid;

void readPayload();
void readAdaptationField();
void readTsHeader();

#endif
