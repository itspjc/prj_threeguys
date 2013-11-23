
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "header/scheduler.h"

#define MAXSCHEDULE 20
#define INVALID_SCHEDULE_ID ((u_long)-1)

static u_long next_schedule_id = 0;

static struct SCHEDULE_LIST
{
    u_long id;
    u_long time;
    u_long session_id;
    void (*fn)(void *);
    void *arg;
} schedule_list[MAXSCHEDULE];

void
schedule_init()    // scheduler 생성 
{
    int i;

    for(i=0;i<MAXSCHEDULE;i++)
        schedule_list[i].id = INVALID_SCHEDULE_ID; // 뭐하는 schedule_list 배열일까...
}

u_long 
schedule_enter(u_long time, u_long session_id, void (*fn)(void*), void* arg)
{
    int i;

    for(i=0;i<MAXSCHEDULE;i++)
    {
        if(schedule_list[i].id == INVALID_SCHEDULE_ID)
        {
            schedule_list[i].id = ++next_schedule_id;
            schedule_list[i].time = time;
            schedule_list[i].session_id = session_id;
            schedule_list[i].fn = fn;
            schedule_list[i].arg = arg;
            return schedule_list[i].id;
	}
    }
    return INVALID_SCHEDULE_ID;
}  // 처음으로 id가 -1인 애가 나오면 id를 할당하고 input argu.인 time, session id, stream_event, streamer을 대입(streamer.c 의 start_stream에서 사용함

void
schedule_remove(u_long id)
{
    int i;

    for(i=0;i<MAXSCHEDULE;i++)
    {
        if(schedule_list[i].id == id)
        {
            schedule_list[i].id = INVALID_SCHEDULE_ID;
            return;
	}
    }
}// 해당 schedule id의 schedule을 remove

void
schedule_remove_session(u_long session_id)
{
    int i;

    for(i=0;i<MAXSCHEDULE;i++)
    {
        if(schedule_list[i].session_id == session_id)
            schedule_list[i].id = INVALID_SCHEDULE_ID;
    }
}//해당 session id의 schedule을 remove

void
schedule_execute(u_long time)
{

   int i;

   for(i=0;i<MAXSCHEDULE;i++)
   {
      if((schedule_list[i].id != INVALID_SCHEDULE_ID) && 
         (schedule_list[i].time <= time))
      {
         schedule_list[i].fn(schedule_list[i].arg);		// 여기서 RTP packet이 socket에 써짐 
         schedule_list[i].id = INVALID_SCHEDULE_ID; // 한번 실행한 뒤에는 Invalid로 수정
      }
   }
}

int
schedule_check( u_long session_id )
{
   int   i;
   
   for( i = 0; i < MAXSCHEDULE; i++ )
      if (schedule_list[i].session_id == session_id)
         return( 1 );
   return( 0 );
}
