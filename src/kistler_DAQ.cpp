#include "kistler_DAQ.hpp"

namespace Kistler
{

API::Trigger &
API::Trigger::request()
{
    triggerUpon = "request";
    return *this;
};

API::Trigger &
API::Trigger::event(int nb_thresholds, bool meas, int nb_channel, bool falling)
{
    triggerUpon = "event";
    nb_trig = nb_thresholds;
    type = ((meas) ? "meas" : "virt");
    nb_chan = nb_channel;
    edge = (falling) ? "falling" : "rising";
    return *this;
};

API::Trigger &
API::Trigger::time(uint64_t t_s, uint32_t t_ns)
{
    triggerUpon = "time";
    time_s = t_s;
    time_ns = t_ns;
    return *this;
};

API::Trigger &
API::Trigger::duration(uint64_t duration_ns)
{
    triggerUpon = "duration";
    duration_time = duration_ns;
    return *this;
}

API::Trigger &
API::Trigger::preTrigger(uint64_t nb_samples)
{
    pre_trig = nb_samples;
    return *this;
};

API::Trigger &
API::Trigger::postTrigger(uint64_t nb_samples)
{
    post_trig = nb_samples;
    return *this;
};

std::string
API::Trigger::str()
{
    std::string ret = "{ \"triggerUpon\": \"" + triggerUpon + "\"";
    if(triggerUpon == "event")
    {
        ret += ", \"event\": \"skybase.sigmon.threshold-";
        ret += std::to_string(nb_trig) + "_" + type + "-";
        ret += std::to_string(nb_chan) + "_" + edge + "\"";
    }
    if(triggerUpon == "duration")
    {
        ret += ", \"duration\": " + std::to_string(duration_time);
    }
    if(triggerUpon == "time")
    {
        ret += ", \"time\": \"" + std::to_string(time_s) + "." +
               std::to_string(time_ns) + "\"";
    }
    if(pre_trig > 0)
    {
        ret += ", \"preTrigger\": " + std::to_string(pre_trig);
    }
    ret += " }";
    return ret;
};

API::API(std::string ip, int verbose)
    : ESC::CLI(verbose, "Kistler_API"), Communication::HTTP_client(
                                            ip, 80, verbose - 1){};

bool
API::check_msg_error(std::string rep)
{
    std::string key = "\"result\" : ";
    int res = std::stoi(rep.substr(rep.find(key) + key.size()));
    if(res ==0)
      return false;
    else
      {
	 key = "\"error\" : ";
	 std::string err = rep.substr(rep.find(key + "\"") + key.size() + 1);
	 err = err.substr(0, err.find("\""));
	 logln("<"+ESC::fstr("Error", {ESC::FG_RED})+"> " + err,true);
	 
      }
    return true;
}

void
API::enable_config(bool enable)
{
    m_content = "{ \"measurementId\": " + std::to_string(m_measurement_id);
    m_content += ", \"enabled\" : ";
    m_content += (enable ? "true" : "false");
    m_content += " }";
    this->post("/api/daq/measurement/enabled/set", m_content.c_str());
    logln("Measurement " + std::to_string(m_measurement_id) +
          " enabled: " + (enable ? "true" : "false"));
};

void
API::set_config(Trigger &start_trigger, Trigger &stop_trigger)
{

    this->enable_config(false);

    m_content = "{ \"measurementId\": " + std::to_string(m_measurement_id);
    m_content += ", \"startTrigger\": " + start_trigger.str();
    m_content += ", \"stopTrigger\": " + stop_trigger.str();
    m_content += ", \"signalProvider\" : \"daq-provider\""
                 ", \"enabled\" : true}";

    logln("New configuration: " + m_content);
    this->post("/api/daq/measurement/configuration/set", m_content.c_str());
};

std::string
API::new_client()
{
    std::string rep = this->post("/api/daq/stream/register");
    std::string key = "\"clientId\" : ";
    std::string client_id = rep.substr(rep.find(key + "\"") + key.size() + 1);
    client_id = client_id.substr(0, client_id.find("\""));

    logln("New client id: " + client_id, true);
    m_client_ids.push_back(client_id);
    return m_client_ids[m_client_ids.size() - 1];
};

void
API::delete_client(std::string client_id)
{
    if(client_id == "")
        client_id = m_client_ids[m_client_ids.size() - 1];

    m_content = "{ \"clientId\": \"" + client_id + "\" }";
    this->post("/api/daq/stream/unregister", m_content.c_str());
    logln("Client " + client_id + " deleted", true);
};

int
API::open_stream(int port, std::string client_id)
{
    if(client_id == "")
        client_id = m_client_ids[m_client_ids.size() - 1];

    m_content = "{ \"measurements\": [ { \"measurementId\":1,";
    m_content += "\"scansPerFrame\": 1024 }],";
    m_content += "\"port\": " + std::to_string(port) + ",";
    m_content += " \"clientId\": \"" + client_id + "\" }";
    std::string rep = this->post("/api/daq/stream/open", m_content.c_str());
    std::string key = "\"streamId\" : ";

    int streamId = std::stoi(rep.substr(rep.find(key) + key.size()));
    m_stream_ids.push_back(streamId);
    logln("New stream id: " + std::to_string(streamId), true);
    return streamId;
};

void
API::close_stream(int streamId = -1, std::string client_id)
{
    if(streamId == -1)
        streamId = m_stream_ids[m_stream_ids.size() - 1];
    if(client_id == "")
        client_id = m_client_ids[m_client_ids.size() - 1];

    m_content = "{ \"clientId\": \"" + client_id + "\",";
    m_content += "\"streamId\": " + std::to_string(streamId) + " }";
    std::string rep = this->post("/api/daq/stream/close", m_content.c_str());
    logln("Close stream id: " + std::to_string(streamId), true);
};

void
API::start()
{
    m_content = "{ \"measurementId\": 1}";
    this->post("/api/daq/measurement/start", m_content.c_str());
    logln("Started measurement stream", true);
};

void
API::stop()
{
    m_content = "{ \"measurementId\": 1}";
    this->post("/api/daq/measurement/stop", m_content.c_str());
    logln("Stoped measurement stream", true);
};

}; // namespace Kistler
