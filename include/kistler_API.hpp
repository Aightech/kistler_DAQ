#ifndef __KISTLER_API_HPP__
#define __KISTLER_API_HPP__

#include "com_client.hpp"
#include "http_client.hpp"
#include <cstring>

namespace Kistler
{

/**
 * @brief Object to handle the communication with the Kistler REST API
 *
 */
class API : public Communication::HTTP
{
    public:
    /**
     * @brief
     */
    struct Trigger
    {
        /**
         * @brief Set the Trigger object as a request type: start/stop if the API receives a START or STOP request
         *
         * @return Trigger& itself
         */
        Trigger &
        request();

        /**
         * @brief Set the Trigger object as a threshold type: start/stop if the channel value exceeds the threshold
         *
         * @param nb_thresholds Number of thresholds to monitor
         * @param meas Channel type: Measure (true) or Virtual (false)
         * @param nb_channel Number of channel to monitor
         * @param falling Slope: Falling (true) or Rising (false)
         * @return Trigger& itself
         */
        Trigger &
        event(int nb_thresholds = 1,
              bool meas = true,
              int nb_channel = 1,
              bool falling = true);

        /**
         * @brief Set the Trigger object as a duration type: stop if after the given amount of time
         *
         * @param duration_ns The duration in nanoseconds
         * @return Trigger& itself
         */
        Trigger &
        duration(uint64_t duration_ns);

        /**
         * @brief Set the Trigger as a Time type: start/stop if the given time is reached
         *
         * @param t_s The time: seconds part
         * @param t_ns The time: nanoseconds part
         * @return Trigger& itself
         */
        Trigger &
        time(uint64_t t_s, uint32_t t_ns);
        
        /**
         * @brief Set the the duration to record before the trigger
         *
         * @param duration_ns The duration in nanoseconds
         * @return Trigger& itself
         */
        Trigger &
        preTrigger(uint64_t duration_ns);

        /**
         * @brief Set the the duration to record after the trigger
         *
         * @param duration_ns The duration in nanoseconds
         * @return Trigger& itself
         */
        Trigger &
        postTrigger(uint64_t duration_ns);

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
    API(int verbose = -1);

    /**
     * @brief Set a string parameter of the DAQ via the REST API
     *
     * @param param The parameter to set
     * @param value The value to set
     */
    void
    set_param(std::string param, std::string value);

    /**
     * @brief Set an integer parameter of the DAQ via the REST API
     *
     * @param param The parameter to set
     * @param value The value to set
     */
    void
    set_param(std::string param, int value);

    /**
     * @brief Set a float parameter of the DAQ via the REST API
     *
     * @param param The parameter to set
     * @param value The value to set
     */
    void
    set_param(std::string param, bool value);

    /**
     * @brief Get the value of a parameter of the DAQ via the REST API
     *
     * @param param The parameter to get
     * @return std::string The value of the parameter
     */
    std::string
    get_param(std::string param);

    /**
     * @brief Check the satue of the reply (result:0 => OK)
     *
     * @param rep The reply to check
     * @return true If the reply is OK (result:0)
     * @return false If the reply is an error (result:1)
     */
    bool
    check_msg_error(std::string rep);

    /**
     * @brief Enable or disable the measurement configuration of the DAQ. To set a new configuration, the measurement must be disabled.
     *
     * @param enable true to enable the measurement, false to disable it
     */
    void
    enable_config(bool enable = true);

    /**
     * @brief Set the configuration of the DAQ.
     *
     * @param start_trigger The trigger to start the measurement
     * @param stop_trigger The trigger to stop the measurement
     */
    void
    set_config(Trigger &start_trigger, Trigger &stop_trigger);

    /**
     * @brief Register a new client and get its ID
     *
     * @return std::string The ID of the client
     */
    std::string
    new_client();

    /**
     * @brief Unregister a client
     *
     * @param client_id The ID of the client to unregister
     */
    void
    delete_client(std::string client_id = "");

    /**
     * @brief Open a streaming port. The port will be closed if no connection is made within 30 seconds.
     *
     * @param port The port to open
     * @param client_id The ID of the client
     * @param frame_size The size of the frame (the number of samples to send for each channel at each iteration)
     * @return int the stream ID
     */
    int
    open_stream(int* port, std::string client_id = "", int frame_size = 1024);

    /**
     * @brief Close a streaming port
     *
     * @param streamId The ID of the stream to close
     * @param client_id The ID of the client
     */
    void
    close_stream(int streamId, std::string client_id = "");

    /**
     * @brief Send a START request to the DAQ
     **/
    void
    start();

    /**
     * @brief Send a STOP request to the DAQ
     **/
    void
    stop();

    private:
    /**
     * @brief Set the value of a parameter of the DAQ via the REST API
     * 
     * @param param The parameter to set
     * @param field The value to set
     */
    void
    _set_param(std::string param, std::string field);

    std::string m_content;
    int m_measurement_id = 1;
    std::vector<std::string> m_client_ids;
    std::vector<int> m_stream_ids;
};

}; // namespace Kistler

#endif /* __KISTLER_API_HPP__ */
