#ifndef __RTP__H__
#define __RTP__H__

#include <stdint.h>

#define RTP_PAYLOAD_MPEG2_TS 	33
#define TS_PACKET_SIZE			188

typedef struct RTPPacketHeader{	
  	/* byte 0 */
  	uint8_t version:2;
  	uint8_t padding:1;
  	uint8_t extension:1;
	uint8_t csrc_len:4;
  	/* byte 1 */
  	uint8_t marker:1;
  	uint8_t payload:7;
  	/* bytes 2, 3 */
  	uint16_t seq_no;
  	/* bytes 4-7 */
  	uint32_t timestamp;
  	/* bytes 8-11 */
  	uint32_t ssrc;			 
} RTP_HDR;

typedef struct RTPPacket{
	RTP_HDR header;
	char data[5][TS_PACKET_SIZE];
} RTP_PKT;

RTP_PKT rtp_pkt;

#endif
