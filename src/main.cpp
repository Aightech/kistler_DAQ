#include <iostream>
#include <lsl_c.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "audio.hpp"
#include "kistler_DAQ.hpp"
#include <atomic>
#include <mutex>

struct AudioBuffer
{
    float *data;
    std::mutex *mutex;
    int new_data = 0;
    int pos_w=0;
    int pos_r=-1;
    int nb_page=7;
    struct timespec ts={};
    time_t now_s=0;
    uint64_t now_ns=0;
    int size=0;

    AudioBuffer(int buffer_size)
    {
        data = new float[buffer_size];
        size = buffer_size;
        mutex = new std::mutex();
    }

    void
    write(float *src, int nb_sample)
    {
        std::lock_guard<std::mutex> lock(*mutex);
        int s = (nb_sample> size-pos_w)?size-pos_w:nb_sample;
            memcpy(data+pos_w, src, s*4);
        if(s<nb_sample)
            memcpy(data, src+s, (nb_sample-s)*4);
        pos_w+=nb_sample;
        pos_w=(pos_w<size)?pos_w:0;
        new_data+=nb_sample;
    };
    void
    read(float *dst, int nb_sample)
    {
        std::lock_guard<std::mutex> lock(*mutex);
        if(pos_r==-1)
            {
                pos_r=(pos_w==0)?nb_page-1:pos_w-1;
                for(int i =0;i<1024*2;i++)
                    pos_r=(pos_r==0)?nb_page-1:pos_r-1;
            }
        int s = (nb_sample> size-pos_r)?size-pos_r:nb_sample;
        memcpy(dst, data+pos_r, s*4);
        if(s<nb_sample)
            memcpy(dst+s, data, (nb_sample-s)*4);
        pos_r+=nb_sample;
        pos_r=(pos_r<size)?pos_r:0;
        new_data-=nb_sample;
    };

    void disp_buff()
    {
        
    clock_gettime(CLOCK_REALTIME, &ts);
    time_t diff_s = ts.tv_sec-now_s;
    uint64_t diff_ns = ts.tv_nsec - now_ns + ((diff_s>0)?1000000000:0);
     now_s = ts.tv_sec;
     now_ns = ts.tv_nsec;
    printf("%ld.%09ld %09ld", now_s, now_ns, diff_ns);
    printf(" %d [", new_data);
        for(int i =0;i<nb_page;i++)
            printf(" %c |", (i==pos_r)?((i==pos_w)?'X':'R'):((i==pos_w)?'W':' '));
        printf("\b]\n");
    }
};

// callback function to be called when a frame is received
// this function will push the data to the outlet
void
callback(uint32_t nb_channel,
         uint32_t nb_sample,
         uint64_t init_timestamp_s,
         uint32_t init_timestamp_ns,
         float *data,
         void *user_data)
{

    // //computer timestamp
    // time_t utc_now = time( NULL );
    // //get utc time in nanoseconds
    // struct timespec ts;
    // clock_gettime(CLOCK_REALTIME, &ts);
    // uint64_t utc_now_ns = ts.tv_nsec;
    // uint64_t diff_last_timestamp_ns = (nb_sample - 1)/12500.*1000000000;

    // // std::cout << "timestampC: " << utc_now << "s " << utc_now_ns << "ns"
    // //           << std::endl;
    // // std::cout << "timestampK: " << init_timestamp_s << "s " << init_timestamp_ns+diff_last_timestamp_ns
    // //           << "ns" << std::endl;
    // //print different between computer and kistler timestamp
    // int64_t diffs = (int64_t)utc_now - (int64_t)init_timestamp_s;
    // int64_t diffns = (int64_t)utc_now_ns - init_timestamp_ns + (diffs>0?1000000000:0)-diff_last_timestamp_ns;
    // std::cout << "diff: " << diffns*0.000001 << "ms" << std::endl;

    AudioBuffer *buffer = (AudioBuffer *)user_data;
    buffer->write(data, nb_sample * nb_channel);

    // lsl_outlet outlet = (lsl_outlet)user_data;
    // lsl_push_chunk_ft(outlet, data, nb_sample * nb_channel, 0);
};

static int
paCallback(const void *inputBuffer,
           void *outputBuffer,
           unsigned long framesPerBuffer,
           const PaStreamCallbackTimeInfo *timeInfo,
           PaStreamCallbackFlags statusFlags,
           void *userData)
{
    float *out = (float *)outputBuffer;
    AudioBuffer *buffer = (AudioBuffer *)userData;
    if(buffer->new_data>0)
        buffer->read(out, framesPerBuffer);
    (void)timeInfo;
    (void)statusFlags;
    return 0;
}

int
main(int argc, char **argv)
{
    Kistler::DAQ *daq = new Kistler::DAQ(4);
    daq->connect("192.168.0.100");
    daq->config({1}, 12500,
                -1); //set the config to stream data until stop signal is sent

    // lsl_streaminfo info =
    //     lsl_create_streaminfo("Kistler", "measurement", daq->nb_channels(),
    //                           daq->sampling_rate(), cft_float32, "Kistleruid");
    // lsl_outlet outlet = lsl_create_outlet(info, daq->frame_size(), 360);

    AudioBuffer buffer(1024*8);

    daq->set_callback(callback, (void *)&buffer);
    daq->start_streaming();

    usleep(1000000);
    //create audio stream
    AudioStream audio;
    audio.config(0, 1, 12500, 127*2, paFloat32, paCallback, (void *)&buffer);
    audio.start();
    std::cout << "\n## Press enter to stop streaming ##" << std::endl;
    char c = getchar();

    daq->stop_streaming();
    delete daq;
    // //destroy the outlet
    // lsl_destroy_outlet(outlet);
    // //destroy the streaminfo object
    // lsl_destroy_streaminfo(info);
    return 0;
}
