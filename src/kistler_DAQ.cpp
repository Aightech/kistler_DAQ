#include "kistler_DAQ.hpp"

namespace Kistler
{

DAQ::DAQ(int verbose)
    : ESC::CLI(verbose, "Kistler_DAQ"), m_api(verbose - 1),
      m_stream(verbose - 1)
{
    this->connect("192.168.0.100");
    this->config(2, 10000, 20000000000);
    this->start_streaming();
};
DAQ::~DAQ()
{
    if(m_streaming_thread != nullptr)
    {
        m_streaming_thread->join();
        delete m_streaming_thread;
    }
};

void
DAQ::connect(std::string ip)
{
    m_ip = ip;
    logln("Connecting to " + m_ip, true);
    m_api.open_connection(ip, 80); //start the conn. with kistler REST API
    logln("ok", true);
};

void
DAQ::config(uint64_t nb_ch,
            uint64_t sps,
            uint64_t dur_ns,
            uint64_t pre_trig,
            uint64_t post_trig,
            uint16_t f_size,
            int port)
{
    m_nb_channels = nb_ch;
    m_sampling_rate = sps;
    m_measurement_duration = dur_ns;
    m_pre_trig = pre_trig;
    m_post_trig = post_trig;
    m_frame_size = f_size;
    m_streaming_port = port;

    // start when start request is sent via start()
    API::Trigger start_trigger = API::Trigger().request();
    API::Trigger stop_trigger;
    if(dur_ns > 0) // stop after duration_ns in nanoseconds
        stop_trigger.duration(dur_ns);
    else // stop when stop request is sent via stop()
        stop_trigger.request();
    if(pre_trig > 0) // time (in ns) to record before the start trigger
        start_trigger.preTrigger(pre_trig);
    if(post_trig > 0) // time (in ns) to record after the stop trigger
        stop_trigger.postTrigger(post_trig);

    logln("Configuring DAQ...", true);

    m_api.set_config(start_trigger, stop_trigger);
};

void
DAQ::start_streaming()
{
    //start the streaming threads
    m_streaming_thread = new std::thread(&DAQ::_streaming_thread, this);
};

void
DAQ::_streaming_thread()
{
    try
    {
        lsl_streaminfo info =
            lsl_create_streaminfo("Kistler", "measurement", m_nb_channels,
                                  m_sampling_rate, cft_float32, "Kistleruid");
        lsl_outlet outlet = lsl_create_outlet(info, m_frame_size, 360);

        m_c_id = m_api.new_client();
        m_s_id = m_api.open_stream(m_streaming_port, m_c_id);
        m_stream.open_connection(Communication::Client::Mode::TCP,
                                 "192.168.0.100", m_streaming_port);
        m_api.start();

        uint16_t type;
        uint32_t size;
        uint64_t timestamp_s;
        uint32_t timestamp_ns;
        float *data = new float[m_nb_channels * m_frame_size];
        int total_samples = 0;
        m_is_streaming = true;
        logln("Start streaming");
        for(; m_is_streaming;)
        {
            this->read_header(&type, &size);
            if(type == 0)//it is an event subframe
            {
                this->read_event(size);
                break;
            }
            else// is is a data subframe 
            {
                total_samples += this->read_measurement(size, &timestamp_s,
                                                        &timestamp_ns, &data);
                lsl_push_chunk_ft(outlet, data, m_nb_channels * m_frame_size,
                                  (double)timestamp_s +
                                      (double)timestamp_ns / 1e9);
            }
        }
        logln("Total samples: " + std::to_string(total_samples));
        lsl_destroy_outlet(outlet);
        m_stream.close_connection();
        m_api.stop();
        delete[] data;
    }
    catch(std::string &msg)
    {
        std::cout << msg << std::endl;
    }
    m_api.close_stream(m_s_id, m_c_id);
    m_api.delete_client(m_c_id);
};
  
void
DAQ::stop_streaming(bool forced)
{
    m_api.stop(); //send stop request via rest api
    if(forced)
      m_is_streaming = false;
    if(m_streaming_thread != nullptr)
    {
        m_streaming_thread->join();
        delete m_streaming_thread;
    }
};

void
DAQ::read_header(uint16_t *type, uint32_t *size)
{
    int n;
    uint8_t buff[16];
    n = m_stream.readS(buff, 16);
    while(n < 16) n += m_stream.readS(buff + n, 16 - n);
    *type = *(uint16_t *)(buff + 2);
    *size = *(uint32_t *)(buff + 4) - 16;
    //the rest is redundant information
};

int
DAQ::read_event(uint32_t size)
{

    std::string err_description[] = {ESC::fstr("ERROR", {ESC::FG_RED}),
                                     ESC::fstr("WARNING", {ESC::FG_YELLOW}),
                                     ESC::fstr("STATUS", {ESC::FG_GREEN}),
                                     ESC::fstr("INFO", {ESC::FG_BLUE})};
    std::string code_description[] = {"CLOSED", "OVERRUN", "TIMESKEW",
                                      "MEASUREMENT SUBSYSTEM RECONFIGURED",
                                      "MEASUREMENT STOPPED"};
    int n;
    uint8_t buff[size];
    n = m_stream.readS(buff, size);
    while(n < size) n += m_stream.readS(buff + n, size - n);

    uint8_t *level = buff;
    //uint8_t *facility = buff + 1;
    uint16_t *code = (uint16_t *)(buff + 2);

    logln("<" + std::string(err_description[*level]) + "> " +
              std::string(code_description[*code]),
          true);
    return *code;
};

uint32_t
DAQ::read_measurement(uint32_t size,
                      uint64_t *init_timestamp_s,
                      uint32_t *init_timestamp_ns,
                      float **data)
{
    int n;
    uint8_t buff[12];
    uint32_t nb_samples = (size - 12) / 4 / m_nb_channels;

    //read timestamp
    n = m_stream.readS(buff, 12);
    while(n < 12) n += m_stream.readS(buff + n, 12 - n);
    *init_timestamp_s = *(uint64_t *)(buff);
    *init_timestamp_ns = *(uint32_t *)(buff + 8);

    //read data
    size -= 12;
    n = m_stream.readS((uint8_t *)*data, size);
    while(n < size) n += m_stream.readS((uint8_t *)*data + n, size - n);

    return nb_samples;
};

}; // namespace Kistler
