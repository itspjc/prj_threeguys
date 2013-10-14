
#ifndef _INTERFACE_H_
#define _INTERFACE_H_

void interface_init(void *);
void interface_new_session(u_long id);
void interface_can_play(u_long id);
void interface_hello(const char *filename);
void interface_error(const char *err);
void interface_can_play(u_long id);
void interface_close_stream(u_long session_id);
void interface_stream_output(u_long id, u_short n, char *data);
void interface_stream_done(u_long session_id);
void interface_close_session(u_long id);
void interface_error( const char *msg );
void interface_alert_msg( const char *msg );
void interface_renew_play( u_long id );
void interface_start_play( u_long id );


#endif /* _INTERFACE_H_ */
