#include <iostream>
#include <lsl_c.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "kistler_DAQ.hpp"
#include <atomic>
#include <mutex>

#include <boost/program_options.hpp>

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
    //display the timestamp of the frame and the value once every 1000 frames
    static int count = 0;
    if(count++ % 2 == 0)
    {
        printf("\xd[%lf] ( ",
               init_timestamp_s + init_timestamp_ns * 0.000000001);
        for(int i = 0; i < nb_channel; i++)
        {
            printf("%s%0.3f ", (data[i] < 0) ? "" : " ", data[i]);
        }
        printf(")");
        fflush(stdout);
    }
    lsl_outlet *outlet = (lsl_outlet *)user_data;
    lsl_push_chunk_ft(*outlet, data, nb_sample * nb_channel,
                      init_timestamp_s + init_timestamp_ns * 0.000000001);
    //std::cout << "pushed chunk " << nb_sample * nb_channel << std::endl;
};

namespace po = boost::program_options;
int
main(int argc, char **argv)
{
    //set possible arguments
    po::options_description desc("Allowed options");
    desc.add_options()("help,h", "produce help message")(
        "ip,i", po::value<std::string>()->default_value("192.168.0.100"),
        "set the ip of the DAQ")(
        "channel_ids,n",
        po::value<std::vector<int>>()->multitoken()->default_value({1, 2, 3, 4},
                                                                   "1 2 3 4"),
        "set the channel to stream")("sampling_rate,s",
                                     po::value<int>()->default_value(12500),
                                     "set the sampling rate")(
        "streaming_time,t", po::value<int>()->default_value(-1),
        "set the streaming time in nanoseconds. -1 for infinite streaming")(
        "verbose,v", po::value<int>()->default_value(1), "Verbose level");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if(vm.count("help"))
    {
        //print basic usage and description
        std::cout << "Usage: " << argv[0] << " [options]\n";
        std::cout << desc << "\n";
        return 1;
    }

    int verbose_lvl = vm["verbose"].as<int>();
    std::string ip = vm["ip"].as<std::string>();
    std::vector<int> channels = vm["channel_ids"].as<std::vector<int>>();
    int sampling_rate = vm["sampling_rate"].as<int>();
    int duration = vm["streaming_time"].as<int>();

    Kistler::DAQ *daq = new Kistler::DAQ(verbose_lvl);
    daq->connect(ip);
    daq->config(
        channels, sampling_rate,
        duration); //set the config to stream data until stop signal is sent

    lsl_streaminfo info =
        lsl_create_streaminfo("Kistler", "measurement", daq->nb_channels(),
                              daq->sampling_rate(), cft_float32, "Kistleruid");
    lsl_outlet outlet = lsl_create_outlet(info, daq->frame_size(), 360);


    std::cout << "\n\n## Press enter to stop streaming ##\n" << std::endl;

    daq->set_callback(callback, (void *)&outlet);
    daq->start_streaming();
    usleep(1000000);

    char c = getchar();

    daq->stop_streaming();
    delete daq;
    //destroy the outlet
    lsl_destroy_outlet(outlet);
    //destroy the streaminfo object
    lsl_destroy_streaminfo(info);
    return 0;
}
