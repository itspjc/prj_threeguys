
#ifndef _RTSP_AUDIO_
#define _RTSP_AUDIO_

struct RTSP_AUDIO_data
{
    u_long codec_id;
    u_short num_channels;
    u_short bits_per_sample;
    u_long samples_per_sec;
    u_short ave_frame_size;
    u_short max_frame_size;
    u_long samples_per_frame;
    u_char packet;
    u_short frames_per_packet;
    u_short extra_bytes;
    u_long num_frames;
    char *extra_data;
};

#endif /* _RTSP_AUDIO_ */
