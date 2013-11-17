
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>

#include "header/machdefs.h"
#include "header/socket.h"
#include "header/config.h"
#include "header/eventloop.h"
#include "header/msg_handler.h"
#include "header/socket.h"
#include "header/scheduler.h"
#include "header/server.h"
#include "header/util.h"
#include "header/rtsp.h"
#include "header/streamer.h"
#include "header/session.h"
#include "header/parse.h"
#include "header/rtp.h"

/******************************************
 *     received message handler functions
 ******************************************/

int
handle_setup_request(char *b)
{
	return 1;
}

int
handle_play_request(char *b)
{
    return 1;
}

int
handle_pause_request(char *b)
{
	return 1;
}

int
handle_close_request(void)
{
	return 1;
}

/***************************************
 *         server state machine
 **************************************/

void
handle_event(int event, int status, char *buf)
{
    static char *sStateErrMsg = "State error: event %d in state %d\n";
	printf(buf);
    switch (s_state.cur_state) {
    case INIT_STATE: // INIT_STATE = 0
    {
        switch (event) {
        case RTSP_GET_METHOD:
//	        handle_get_request(buf);
	        break;

        case RTSP_SETUP_METHOD:
//	        if (handle_setup_request(buf))
	            s_state.cur_state = READY_STATE;
	        break;

        case RTSP_CLOSE_METHOD:
//	        handle_close_request();
//	        DescribedFlag = 0;
	        break;

        case RTSP_HELLO_METHOD:
//	        handle_hello_request();
	        break;

        case RTSP_HELLO_RESPONSE:
//	        handle_hello_reply(status);
	        break;

        case RTSP_PLAY_METHOD:
        case RTSP_PAUSE_METHOD:
	        send_reply(455, "Accept: OPTIONS, DESCRIBE, SETUP, TEARDOWN\n");
	        break;

        default:
//	        printf(sStateErrMsg, event, s_state.cur_state);
//	        discard_msg();	
//	        send_reply(501, "Accept: OPTIONS, DESCRIBE, SETUP, TEARDOWN\n");
	        break;
        }
        break;
    }				/* INIT state */

    case READY_STATE:
    {
        switch (event) {
            case RTSP_PLAY_METHOD:
	            if (handle_play_request(buf))
	                s_state.cur_state = PLAY_STATE;
	            break;

            case RTSP_SETUP_METHOD:
//	            if (!handle_setup_request(buf))
//	                s_state.cur_state = INIT_STATE;
	            break;

            case RTSP_CLOSE_METHOD:
//	            if (handle_close_request())
//	                s_state.cur_state = INIT_STATE;
//	            DescribedFlag = 0;
	            break;

            case RTSP_HELLO_METHOD:
//	            handle_hello_request();
	            break;

            case RTSP_PAUSE_METHOD:	/* method not valid this state. */
//	            send_reply(455, "Accept: OPTIONS, SETUP, PLAY, TEARDOWN\n");
	            break;

            case RTSP_GET_METHOD:
//	            handle_get_request(buf);
	            break;

            case RTSP_HELLO_RESPONSE:
//	            handle_hello_reply(status);
	            break;

            default:
//	            printf(sStateErrMsg, event, s_state.cur_state);
//	            discard_msg();
//	            send_reply(501, "Accept: OPTIONS, SETUP, PLAY, TEARDOWN\n");
	            break;
        }
        break;
    }				/* READY state */

    case PLAY_STATE:
    {
        switch (event) {
            case RTSP_PLAY_METHOD:
//	            if (!handle_play_request(buf))
//	                s_state.cur_state = READY_STATE;
	            break;

            case RTSP_PAUSE_METHOD:
//	            if (handle_pause_request(buf))
//	                s_state.cur_state = READY_STATE;
	            break;

            case RTSP_CLOSE_METHOD:
//	            if (handle_close_request())
//	                s_state.cur_state = INIT_STATE;
//	            DescribedFlag = 0;
	            break;

            case RTSP_HELLO_METHOD:
//	            handle_hello_request();
	            break;

            case RTSP_HELLO_RESPONSE:
//	            handle_hello_reply(status);
	            break;

            case RTSP_GET_METHOD:
//	            if (!handle_get_request(buf))
//	                s_state.cur_state = INIT_STATE;
	            break;

	        case RTSP_SETUP_METHOD:
//	            if (handle_setup_request(buf))
//	                s_state.cur_state = READY_STATE;
//	            else
//		            s_state.cur_state = INIT_STATE;
	            break;

            default:
//	            printf(sStateErrMsg, event, s_state.cur_state);
//	            discard_msg();
//	            send_reply(501, "Accept: OPTIONS, SETUP, PLAY, TEARDOWN\n");
	            break;
        }
	    break;
    }				/* PLAY state */

    default: {
//        printf("State error: unknown state=%d, event code=%d\n",
//	    s_state.cur_state, event);
  //      discard_msg();	
        }
        break;
    }				/* end of current state switch */
}
