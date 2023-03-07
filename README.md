# kistler_DAQ

The Kistler API is a C++ library that provides functionality to interface with a Kistler DAQ (Data Acquisition System) device. This library allows the user to connect to the DAQ, configure it, and start streaming data. The data can be streamed to Local Area Network (LAN) using LSL (Lab Streaming Layer) protocol.
## How to use

The Kistler_API header file provides a DAQ class which can be used to interface with the Kistler DAQ device. This class contains various methods that allow the user to interact with the device.

To use the DAQ class, simply include the kistler_API.hpp header file in your code. Then, create an instance of the DAQ class and use the available methods to connect to the device, configure it, and start streaming data.

# Example
Open the ![main.cpp](cpp:src/main.cpp) file to get an example how to use the lib.
sudo apt install linuxptp chrony ethtool


```cpp
#include "kistler_API.hpp"
#include <lsl_c.h>
#include <thread>

Kistler::DAQ kistler_device;

int main(int argc, char **argv)
{
    // Connect to the device
    kistler_device.connect("192.168.0.100");

    // Configure the device
    kistler_device.config();

    // Start streaming data
    kistler_device.start_streaming();

    // Wait for a few seconds
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Stop streaming data
    kistler_device.stop_streaming();
    
    return 0;
}
```
This code snippet creates an instance of the DAQ class, connects to the Kistler DAQ device at IP address "192.168.0.100", configures it with default settings, starts streaming data, waits for 5 seconds, and then stops streaming data.

# Building source code

To build the project run:
```bash
cd kistler_DAQ
mkdir build && cd build
cmake .. && make
```

# Demonstration app

When the project have been built, you can run:
```bash
./kistler_DAQ -h
```
to get the demonstration app usage.

