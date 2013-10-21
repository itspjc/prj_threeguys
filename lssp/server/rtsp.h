
#ifndef _RTSP_H_
#define _RTSP_H_

#define RTSP_DEFAULT_PORT		554

#define RTSP_VER  "RTSP/1.0"

/* Description Formats */
#define DESCRIPTION_SDP_FORMAT 1
#define DESCRIPTION_MH_FORMAT 0

/* Session IDs */
#define RTSP_COMMAND_SESSION        0
#define RTSP_FIRST_CLIENT_SESSION	1024
#define RTSP_FIRST_SERVER_SESSION	1025

/* HELLO parameters */
#define RTSP_MAJOR_VERSION              1
#define RTSP_MINOR_VERSION              0

/*  Transport types for SET_TRANSPORT */
#define RTSP_TRANSTYPE_UNICASTUDP 	1	/* Unicast UDP */
#define RTSP_TRANSTYPE_MULTICASTUDP 	2	/* Multicast UDP */
#define RTSP_TRANSTYPE_SCP 		3       /* SCP */
#define RTSP_TRANSTYPE_SCPCOMPRESSED 	4	/* SCP Compressed */

/*  Transport speeds for normal speed */
#define RTSP_TRANSPEED_NORMAL 		65536

/*  Report types for SEND_REPORT and REPORT messages */
#define RTSP_REPORTTYPE_NONE 		0       /* No report */
#define RTSP_REPORTTYPE_PING 		1       /* PING	 */
#define RTSP_REPORTTYPE_TEXT 		2       /* Text Message	 */
#define RTSP_REPORTTYPE_RECEPTION 	3       /* Reception Report */

/*  Lagging parameters for REPORTTYPE_RECEPTION */
#define RTSP_LAGGING_ABOVE 		0
#define RTSP_LAGGING_BELOW 		1

/*  Parameter Families for SET_PARAM, PARAM_REPLY, etc. */
#define RTSP_FAMILY_AUDIO 		1

/*  Parameters in RTSP_FAMILY_AUDIO */
#define RTSP_PARAM_AUDIOFMTDESC		1   /* Audio Format Descriptor */
#define RTSP_PARAM_AUDIOANNOTATIONS	2   /* Audio Annotations */


#endif /* _RTSP_H_ */
