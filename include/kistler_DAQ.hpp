#ifndef __KISTLER_DAQ_HPP__
#define __KISTLER_DAQ_HPP__

#include "com_client.hpp"
#include "http_client.hpp"
#include <cstring>

namespace Kistler
{

class API : public Communication::HTTP_client
{
    public:
    struct Trigger
    {
        Trigger &
        request();

        Trigger &
        event(int nb_thresholds = 1,
              bool meas = true,
              int nb_channel = 1,
              bool falling = true);

        Trigger &
        duration(uint64_t duration_ns);

        Trigger &
        time(uint64_t t_s, uint32_t t_ns);
        ;

        Trigger &
        preTrigger(uint64_t nb_samples);

        Trigger &
        postTrigger(uint64_t nb_samples);

        std::string
        str();

        std::string triggerUpon;
        int nb_trig = 0;
        int nb_chan = 0;
        uint64_t post_trig = 0;
        uint64_t pre_trig = 0;
        uint64_t duration_time = 0;
        std::string type;
        std::string edge;
        uint32_t time_s = 0;
        uint64_t time_ns = 0;
    };

    public:
    API(std::string ip, int verbose = -1);

    bool
    check_msg_error(std::string rep);

    void
    enable_config(bool enable = true);

    void
    set_config(Trigger &start_trigger, Trigger &stop_trigger);

    std::string
    new_client();

    void
    delete_client(std::string client_id = "");

    int
    open_stream(int port, std::string client_id = "");

    void
    close_stream(int streamId, std::string client_id = "");

    void
    start();

    void
    stop();

    private:
    int m_measurement_id = 1;
    std::vector<std::string> m_client_ids;
    std::vector<int> m_stream_ids;
};

class DAQ : virtual public ESC::CLI
{
    struct Measurement
    {
        Measurement(uint64_t nb_ch,
                    uint64_t sps,
                    uint64_t dur_ns,
                    uint16_t f_size)
        {
            nb_channels = nb_ch;
            sampling_rate = sps;
            duration_ns = dur_ns;
            frame_size = f_size;
            nb_samples = duration_ns * sampling_rate / 1e9;

	    data = new float[nb_channels * nb_samples];
	    timestamps_s = new uint64_t[nb_samples/frame_size];
	    timestamps_ns = new uint32_t[nb_samples/frame_size];
        }

        uint64_t nb_samples;
        uint32_t nb_channels;
        uint64_t duration_ns;
        uint64_t sampling_rate;
        uint16_t frame_size;
        float *data;
        uint64_t *timestamps_s;
        uint32_t *timestamps_ns;
    };

    public:
    DAQ(std::string ip = "192.168.0.100", int verbose = 1)
        : ESC::CLI(verbose, "Kistler_DAQ"), m_ip(ip), m_api(ip, 3), m_stream(2)
    {
        uint64_t duration = 2000000000;
        m_api.set_config(API::Trigger().request(),
                         API::Trigger().duration(duration));
        std::string c_id = m_api.new_client();
        int s_id = m_api.open_stream(m_streaming_port, c_id);

        this->start_streaming();
        this->stop_streaming();

        m_api.close_stream(s_id, c_id);
        m_api.delete_client(c_id);
    };
    ~DAQ(){
        // Close the DAQ
        // ...

    };

    void
    start_streaming()
    {
        try
        {
            m_stream.open_connection(Communication::Client::Mode::TCP,
                                     "192.168.0.100", m_streaming_port);
            m_api.start();

            uint16_t type;
            uint32_t size;
            Measurement data;
            uint64_t timestamp_s;
            uint32_t timestamp_ns;
            int total_samples = 0;
            for(;;)
            {
                this->read_header(&type, &size);
                if(type == 0)
                {
                    this->read_event(size);
                    break;
                }
                else
                {
                    total_samples += this->read_measurement(
                        size, &timestamp_s, &timestamp_ns, data);
                }
            }
            logln("Total samples: " + std::to_string(total_samples));
        }
        catch(std::string &msg)
        {
            std::cout << msg << std::endl;
        }
    };

    void
    stop_streaming()
    {
        //m_api.stop();
        m_stream.close_connection();
    };

    void
    read_header(uint16_t *type, uint32_t *size)
    {
        int n;
        uint8_t buff[16];
        n = m_stream.readS(buff, 16);
        while(n < 16) n += m_stream.readS(buff + n, 16 - n);
        *type = *(uint16_t *)(buff + 2);
        *size = *(uint32_t *)(buff + 4) - 16;
    };
    int
    read_event(uint32_t size)
    {
        int n;
        uint8_t buff[size];
        n = m_stream.readS(buff, size);
        while(n < size) n += m_stream.readS(buff + n, size - n);

        uint8_t *level = buff;
        //uint8_t *facility = buff + 1;
        uint16_t *code = (uint16_t *)(buff + 2);

        std::string err_description[] = {ESC::fstr("ERROR", {ESC::FG_RED}),
                                         ESC::fstr("WARNING", {ESC::FG_YELLOW}),
                                         ESC::fstr("STATUS", {ESC::FG_GREEN}),
                                         ESC::fstr("INFO", {ESC::FG_BLUE})};
        std::string code_description[] = {"CLOSED", "OVERRUN", "TIMESKEW",
                                          "MEASUREMENT SUBSYSTEM RECONFIGURED",
                                          "MEASUREMENT STOPPED"};

        logln("<" + std::string(err_description[*level]) + "> " +
                  std::string(code_description[*code]),
              true);
        return *code;
    };

    uint32_t
    read_measurement(uint32_t size,
                     uint64_t *init_timestamp_s,
                     uint32_t *init_timestamp_ns,
                     Measurement &data)
    {
        int n;
        uint8_t buff[size];
        n = m_stream.readS(buff, size);
        while(n < size) n += m_stream.readS(buff + n, size - n);

        uint32_t nb_samples = (size - 8 - 4) / 4 / data.nb_channels;
        *init_timestamp_s = *(uint64_t *)(buff);
        *init_timestamp_ns = *(uint32_t *)(buff + 8);
        uint8_t *sub_frame = buff + 8 + 4;

        // for(int i = 0; i < nb_samples; i++)
        // {
        //     for(int j = 0; j < data.size(); j++)
        //         data[j].push_back(
        //             *(float_t *)(sub_frame + 4 * (data.size() * i + j)));
        // }

        logln("Timestamp: " + std::to_string(*init_timestamp_s) + "." +
              std::to_string(*init_timestamp_ns) + " (" +
              std::to_string(nb_samples) + " samples)");

        return nb_samples;
    };

    private:
    std::string m_ip;
    int m_streaming_port = 5000;
    API m_api;
    Communication::Client m_stream;
};

}; // namespace Kistler

#endif /* __KISTLER_DAQ_HPP__ */
