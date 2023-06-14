#include <iostream>
#include <lsl_c.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "kistler_DAQ.hpp"
#include <atomic>
#include <mutex>


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

    //computer timestamp
    time_t utc_now = time( NULL );
    //get utc time in nanoseconds
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    uint64_t utc_now_ns = ts.tv_nsec;
    uint64_t diff_last_timestamp_ns = (nb_sample - 1)/12500.*1000000000;

    // std::cout << "timestampC: " << utc_now << "s " << utc_now_ns << "ns"
    //           << std::endl;
    // std::cout << "timestampK: " << init_timestamp_s << "s " << init_timestamp_ns+diff_last_timestamp_ns
    //           << "ns" << std::endl;
    //print different between computer and kistler timestamp
    int64_t diffs = (int64_t)utc_now - (int64_t)init_timestamp_s;
    int64_t diffns = (int64_t)utc_now_ns - init_timestamp_ns + (diffs>0?1000000000:0)-diff_last_timestamp_ns;
    std::cout << "diff: " << diffns*0.000001 << "ms" << std::endl;


    lsl_outlet outlet = (lsl_outlet)user_data;
    lsl_push_chunk_ft(outlet, data, nb_sample * nb_channel, 0);
};


int
main(int argc, char **argv)
{
    Kistler::DAQ *daq = new Kistler::DAQ(4);
    daq->connect("192.168.0.100");
    daq->config({1}, 12500,
                -1); //set the config to stream data until stop signal is sent

    lsl_streaminfo info =
        lsl_create_streaminfo("Kistler", "measurement", daq->nb_channels(),
                              daq->sampling_rate(), cft_float32, "Kistleruid");
    lsl_outlet outlet = lsl_create_outlet(info, daq->frame_size(), 360);


    daq->set_callback(callback, (void *)&outlet);
    daq->start_streaming();

    usleep(1000000);
    
    std::cout << "\n## Press enter to stop streaming ##" << std::endl;
    char c = getchar();

    daq->stop_streaming();
    delete daq;
    //destroy the outlet
    lsl_destroy_outlet(outlet);
    //destroy the streaminfo object
    lsl_destroy_streaminfo(info);
    return 0;
}
