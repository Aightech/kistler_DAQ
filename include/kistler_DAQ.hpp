#ifndef __KISTLER_DAQ_HPP__
#define __KISTLER_DAQ_HPP__

#include "kistler_API.hpp"
#include <cstring>
#include <thread>

namespace Kistler
{

class DAQ : virtual public ESC::CLI
{
    public:
    DAQ(int verbose = 3);
    ~DAQ();

    /**
     * @brief Connect to the DAQ
     * @param ip IP address of the DAQ
     * @param port Port of the DAQ
     */
    void
    connect(std::string ip = "192.168.0.100");

    /**
     * @brief Configure the DAQ
     * @param nb_ch Number of channels to be recorded
     * @param sps Sampling rate
     * @param dur_ns Duration of the recording in nanoseconds
     * @param pre_trig Pre-trigger duration in nanoseconds
     * @param post_trig Post-trigger duration in nanoseconds
     * @param f_size Size of each frame (number of sample sent for each channel each time)
     * @param port Port of the streaming socket
     */
    void
    config(std::initializer_list<int> channels,
           uint64_t sps = 10000,
           int64_t dur_ns = -1,
           uint64_t pre_trig = 0,
           uint64_t post_trig = 0,
           uint16_t f_size = 1024,
           int port = 5001);

    /**
     * @brief Start a LSL streaming thread
     */
    void
    start_streaming();

    /**
     * @brief Thread streaming LSL samples
     */
    void
    _streaming_thread();

    /**
     * @brief Stop the streaming thread
     */
    void
    stop_streaming(bool forced = false);

    /**
     * @brief read the header of an incoming frame
     **/
    void
    read_header(uint16_t *type, uint32_t *size);

    /**
     * @brief Read a subframe as an event (type 0 in header)
     * @param size Size of the subframe
     **/
    int
    read_event(uint32_t size);

    /**
     * @brief Read a subframe as a data frame (type 1 in header)
     * @param size Size of the subframe
     **/
    uint32_t
    read_measurement(uint32_t size,
                     uint64_t *init_timestamp_s,
                     uint32_t *init_timestamp_ns);

    void
    set_callback(void (*callback)(uint32_t, uint32_t, uint64_t, uint32_t, float *, void*), void *user_data = nullptr)
    {
        m_callback = callback;
        m_user_data = user_data;
    }

    int nb_channels() { return m_nb_channels; }
    uint64_t sampling_rate() { return m_sampling_rate; }
    uint64_t measurement_duration() { return m_measurement_duration; }
    int frame_size() { return m_frame_size; }

    private:
    std::string m_ip;
    int m_nb_channels;
    uint64_t m_sampling_rate;
    uint64_t m_measurement_duration;
    uint64_t m_pre_trig = 0;
    uint64_t m_post_trig = 0;
    int m_streaming_port = 5000;
    int m_frame_size = 1024;
    API m_api;
    Communication::Client m_stream;
    std::string m_c_id;
    int m_s_id;
    bool m_is_streaming = false;
    std::thread *m_streaming_thread = nullptr;

    //callback when data is received
    void (*m_callback)(uint32_t, uint32_t, uint64_t, uint32_t, float *, void*) = nullptr;//callback when data is received (nb_channel, nb_sample/channel, timestamp_s, timestamp_ns, data, user_data)
    void *m_user_data = nullptr;

    std::string err_description[4] = {ESC::fstr("ERROR", {ESC::FG_RED}),
                                     ESC::fstr("WARNING", {ESC::FG_YELLOW}),
                                     ESC::fstr("STATUS", {ESC::FG_GREEN}),
                                     ESC::fstr("INFO", {ESC::FG_BLUE})};
    std::string code_description[5] = {"CLOSED", "OVERRUN", "TIMESKEW",
                                      "MEASUREMENT SUBSYSTEM RECONFIGURED",
                                      "MEASUREMENT STOPPED"};
    float* m_data;//[16384];
};

}; // namespace Kistler

#endif /* __KISTLER_DAQ_HPP__ */
