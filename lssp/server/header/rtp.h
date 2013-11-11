
/* payload types */
#define RTP_PAYLOAD_PCMU      0             /* mu-law */
#define RTP_PAYLOAD_PCMA      8             /* a-law */
#define RTP_PAYLOAD_L16_2    10             /* linear 16, 44.1khz, 2 channel */
#define RTP_PAYLOAD_L16_1    11             /* linear 16, 44.1khz, 1 channel */
#define RTP_PAYLOAD_RTSP    101             /* dynamic type, used by this to mean 
                                             * RTSP-negotiated PCM 
                                             */
/* op-codes */
#define RTP_OP_PACKETFLAGS  1               /* opcode datalength = 1 */

#define RTP_OP_CODE_DATA_LENGTH     1

/* flags for opcode RTP_OP_PACKETFLAGS */
#define RTP_FLAG_LASTPACKET 0x00000001      /* last packet in stream */
#define RTP_FLAG_KEYFRAME   0x00000002      /* keyframe packet */

#ifndef BYTE_ORDER
#error BYTE_ORDER not defined
#endif

typedef struct RTPPacketHeader
{
  /* byte 0 */
  u_char csrc_len:4;		/* expect 0 */
  u_char extension:1;		/* expect 1, see RTP_OP below */
  u_char padding:1;		/* expect 0 */
  u_char version:2;		/* expect 2 */
  /* byte 1 */
  u_char payload:7;		/* RTP_PAYLOAD_RTSP */
  u_char marker:1;		/* expect 1 */
  /* bytes 2, 3 */
  u_int16 seq_no;
  /* bytes 4-7 */
  u_int32 timestamp;		/* in ms */
  /* bytes 8-11 */
  u_int32 ssrc;		/* stream number is used here. */
} RTP_HDR;

typedef struct RTPOpCode
{
    u_int16     op_code;                /* RTP_OP_PACKETFLAGS */
    u_int16     op_code_data_length;    /* RTP_OP_CODE_DATA_LENGTH */
    u_int32     op_code_data [RTP_OP_CODE_DATA_LENGTH];
} RTP_OP;

typedef struct RTPData
{
    u_int16     data_length;
    u_char      data [1];       /* actual length determined by data_length */
} RTP_DATA;
