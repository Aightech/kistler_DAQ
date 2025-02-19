#include "kistler_API.hpp"

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

API::API(int verbose)
    : ESC::CLI(verbose, "Kistler_API"), Communication::HTTP(verbose){};

void
API::set_param(std::string param, std::string value)
{
    this->_set_param(param, "\"" + value + "\"");
};

void
API::set_param(std::string param, int value)
{
    this->_set_param(param, std::to_string(value));
};

void
API::set_param(std::string param, bool value)
{
    this->_set_param(param, (value) ? "true" : "false");
};

std::string
API::get_param(std::string param)
{
    std::string content = "{ \"params\": [\"" + param + "\"] }";
    //logln("Getting parameter " + param, true);
    std::string rep = this->post("/api/param/get", content.c_str());
    //logln("Response: " + rep, true);
    this->check_msg_error(rep);
    std::string key = "value\":";
    std::string value = rep.substr(rep.find(key + "\"") + key.size() + 1);
    value = value.substr(0, value.find("\""));
    return value;
};

void
API::_set_param(std::string param, std::string field)
{
    //{"params":[{"name":"/deviceSetup/net/if0/ipv4/static/address","value":"192.168.123.33"}]}'
    logln("Setting parameter " + param + " to " + field, true);
    std::string content = "{ \"params\": [ { \"name\": \"" + param;
    content += "\", \"value\": " + field + " } ] }";
    std::string rep = this->post("/api/param/set", content.c_str());
    this->check_msg_error(rep);
};

bool
API::check_msg_error(std::string rep)
{
    std::string key = "\"result\":";
    int res = std::stoi(rep.substr(rep.find(key) + key.size()));
    if(res == 0)
        return false;
    else
    {
        key = "\"error\": ";
        std::string err = rep.substr(rep.find(key + "{") + key.size() + 1);
        err = err.substr(0, err.find("}"));
        logln("<" + ESC::fstr("Error", {ESC::FG_RED}) + "> " + err, true);
    }
    return true;
}

void
API::enable_config(bool enable)
{
    std::string content = "{ \"measurementId\": " + std::to_string(m_measurement_id);
    content += ", \"enabled\" : ";
    content += (enable ? "true" : "false");
    content += " }";
    logln("Measurement " + (enable?ESC::fstr("ENABLED",{ESC::FG_GREEN}):ESC::fstr("DISABLED",{ESC::FG_YELLOW})), true);
    std::string rep =
        this->post("/api/daq/measurement/enabled/set", content.c_str());
    
    this->check_msg_error(rep);
    
};

void
API::set_config(Trigger &start_trigger, Trigger &stop_trigger)
{

    this->enable_config(false);

    std::string content = "{ \"measurementId\": " + std::to_string(m_measurement_id);
    content += ", \"startTrigger\": " + start_trigger.str();
    content += ", \"stopTrigger\": " + stop_trigger.str();
    content += ", \"signalProvider\" : \"daq-provider\""
                 ", \"enabled\" : true}";

    logln("New configuration: Measurment ID: " + std::to_string(m_measurement_id), true);
    logln("                   Start trigger: " + start_trigger.str(), false);
    logln("                   Stop trigger: " + stop_trigger.str(), false);

    std::string rep =
        this->post("/api/daq/measurement/configuration/set", content.c_str());
    this->check_msg_error(rep);
};

std::string
API::new_client()
{
    std::string rep = this->post("/api/daq/stream/register");
    this->check_msg_error(rep);
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

    std::string content = "{ \"clientId\": \"" + client_id + "\" }";
    std::string rep =
        this->post("/api/daq/stream/unregister", content.c_str());
    this->check_msg_error(rep);
    logln("Client " + client_id + " deleted", true);
};

int
API::open_stream(int* port, std::string client_id, int frame_size)
{
    if(client_id == "")
        client_id = m_client_ids[m_client_ids.size() - 1];

    std::string content = "{ \"measurements\": [ { \"measurementId\":1,";
    content += "\"scansPerFrame\": " + std::to_string(frame_size) + " } ],";
    // content += "\"port\": " + std::to_string(port) + ", ";
    content += " \"clientId\": \"" + client_id + "\" }";
    logln("New stream: Measurement ID: " + std::to_string(m_measurement_id), true);
    // logln("            Port: " + std::to_string(port), false);
    logln("            Client ID: " + client_id, false);
    logln("            Frame size: " + std::to_string(frame_size), false);

    std::string rep = this->post("/api/daq/stream/open", content.c_str());
    this->check_msg_error(rep);
    std::string key = "\"streamId\" : ";
    int streamId = std::stoi(rep.substr(rep.find(key) + key.size()));
    m_stream_ids.push_back(streamId);
    key = "\"port\" : ";
    *port = std::stoi(rep.substr(rep.find(key) + key.size()));
    logln("            New stream id: " + std::to_string(streamId));
    logln("            New port: " + std::to_string(*port));
    return streamId;
};

void
API::close_stream(int streamId = -1, std::string client_id)
{
    if(streamId == -1)
        streamId = m_stream_ids[m_stream_ids.size() - 1];
    if(client_id == "")
        client_id = m_client_ids[m_client_ids.size() - 1];

    std::string content = "{ \"clientId\": \"" + client_id + "\",";
    content += "\"streamId\": " + std::to_string(streamId) + " }";
    std::string rep = this->post("/api/daq/stream/close", content.c_str());
    this->check_msg_error(rep);
    logln("Close stream id: " + std::to_string(streamId), true);
};

void
API::start()
{
    std::string content = "{ \"measurementId\": 1}";
    std::string rep =
        this->post("/api/daq/measurement/start", content.c_str());
    this->check_msg_error(rep);
    logln("Started measurement stream (via API)", true);
};

void
API::stop()
{
    std::string content = "{ \"measurementId\": 1}";
    std::string rep =
        this->post("/api/daq/measurement/stop", content.c_str());
    logln("Stoped measurement stream (via API)", true);
};

}; // namespace Kistler
