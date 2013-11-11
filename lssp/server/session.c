
#include "header/session.h"

void
init_sessions(struct SESSION_STATE *session)
{
    int i;
    for(i=0;i<MAXSESSIONS;i++)
    {
	session[i].session_id = INVALID_SESSION;
	session[i].data = 0;
    }
}

void
add_session(u_long session_id, int state, struct SESSION_STATE *session)
{
    int i;
    for(i=0;i<MAXSESSIONS;i++)
    {
        if(session[i].session_id == INVALID_SESSION)
        {
            session[i].session_id = session_id;
            session[i].cur_state = state;
            session[i].data = 0;
	    return;
        }
    }
}

void
remove_session(u_long session_id, struct SESSION_STATE *session)
{
    int i;
    for(i=0;i<MAXSESSIONS;i++)
    {
        if(session[i].session_id == session_id)
	{
            session[i].session_id = INVALID_SESSION;
	    return;
	}
    }
}

int
get_active_sessions(struct SESSION_STATE *session)
{
    int i;
    int count = 0;
    for(i=0;i<MAXSESSIONS;i++)
    {
        if(session[i].session_id != INVALID_SESSION)
            count++;
    }
    return count;
}

struct SESSION_STATE *
find_session(u_long session_id, struct SESSION_STATE *session)
{
    int i;
    for(i=0;i<MAXSESSIONS;i++)
    {
        if(session[i].session_id == session_id)
	    return &session[i];
    }
    return 0;
}

void
set_session_url(u_long session_id, const char* url, struct SESSION_STATE *session)
{
    struct SESSION_STATE *s = find_session(session_id, session);
    if(s)
        strcpy(s->url, url);
}

const char *
get_session_url(u_long session_id, struct SESSION_STATE *session)
{
    struct SESSION_STATE *s = find_session(session_id, session);
    if(s)
        return s->url;
    return 0;
}


void
set_session_data(u_long session_id, void *data, struct SESSION_STATE *session)
{
    struct SESSION_STATE *s = find_session(session_id, session);
    if(s)
        s->data = data;
}

void *
get_session_data(u_long session_id, struct SESSION_STATE *session)
{
    struct SESSION_STATE *s = find_session(session_id, session);
    if(s)
        return s->data;
    return 0;
}

int
get_session_state(u_long session_id, struct SESSION_STATE *session)
{
    struct SESSION_STATE *s = find_session(session_id, session);
    if(s)
        return s->cur_state;
    return INVALID_SESSION;
}

void
set_session_state(u_long session_id, int state, struct SESSION_STATE *session)
{
    struct SESSION_STATE *s = find_session(session_id, session);
    if(s)
        s->cur_state = state;
}
