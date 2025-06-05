# IMU Parser Project

## Directory Setup
imu_parser_project/ 
├── src/ 
    └── imu_parser_api.cpp 
    └── imu_parser_api.h 
    └── main.cpp 
├── test/ 
│ └── imu_simulator.py 
├── Makefile 
├── README.md
└── build/ # Generated executables (created by make) 
         └── imu_parser

## Overview
This project implements a C++ IMU parser that reads data from a UART device and broadcasts parsed results over the network, along with a Python simulator for testing.

## Problem Statement Requirements
- Parse IMU data from `/dev/tty1` at 921600 baud
- Extract packet data in network byte order (big-endian)
- Execute parsing every 80ms
- Broadcast results on localhost network
- Python simulator for testing

### UART Parser
The code was developed on macoS Ventura 13.7.4, and thus required flexibility to test on this environment while also running on the desired /dev/ttys1 UART on linux. This is apparent in the main code allowing for argument passing of virtual ports and in the API when setting the baud rate.

### Testing Setup
For development and testing, virtual serial ports are recommended:

```bash
# Create virtual serial port pair
socat -d -d pty,raw,echo=0 pty,raw,echo=0
```

or 

```bash
# Create virtual serial port pair
socat -d -d pty,raw,echo=0,cfmakeraw pty,raw,echo=0,cfmakeraw
```

This creates two connected devices (e.g., `/dev/pts/2` and `/dev/pts/3` for linux or `/dev/ttys006` and `dev/ttys007` for mac) that can be used for testing.


## Build and Run

### Building

In the root directory:
```bash
make clean && make
```

### Running with Specified Device (as per requirements)
```bash
sudo ./build/imu_parser  # Requires sudo for /dev/tty1
```
on mac sudo isn't required for using the virtual ports



### Running with Virtual Ports (for testing)
```bash
# Terminal 1: Create virtual ports
socat -d -d pty,raw,echo=0 pty,raw,echo=0

# Terminal 2: Run parser
# NOTE: The ports listed in the arguments might vary based on the
# ports created by the first terminal.

# linux
./build/imu_parser /dev/pts/2

# mac
./build/imu_parser /dev/ttys006

# Terminal 3: Run simulator
cd test
python3 imu_simulator.py

# Terminal 4: Read data from pipe created in terminal 4.
cat /dev/tmp/imu_data | hexdump -C
```

## Code Structure

### Question 1: UART Parsing
- Implemented in `IMUParser` class
- Handles 921600 baud UART communication
- Parses 20-byte packets with 4-byte start frame
- Extracts data in network byte order

### Question 2: Little-Endian Modifications
**Answer**: No modifications needed. The code already handles byte order conversion using `ntohl()` and network-to-host conversion functions. These functions automatically convert from network byte order (big-endian) to host byte order, regardless of whether the host is little-endian or big-endian.

### Question 3: Multi-threaded Environment
- Main loop executes every 80ms using `std::chrono`
- Non-blocking UART reads
- Circular buffer for data handling
- UDP broadcast for network communication

### Question 4: Python Simulator
- Generates realistic IMU test data
- Sends properly formatted packets
- Listens for broadcast responses
- Includes malformed packet testing

## Packet Format
```
Offset | Size | Type            | Description
-------|------|-----------------|------------------
0      | 1    | Raw byte        | Start frame (0x7F)
1      | 1    | Raw byte        | Start frame (0xF0)
2      | 1    | Raw byte        | Start frame (0x1C)
3      | 1    | Raw byte        | Start frame (0xAF)
4      | 4    | Unsigned int    | Packet count
8      | 4    | Float           | X-axis gyro rate
12     | 4    | Float           | Y-axis gyro rate
16     | 4    | Float           | Z-axis gyro rate
```

All multi-byte values are in network byte order (big-endian).

## Network Broadcasting
- Uses UDP broadcast on localhost (127.255.255.255)
- Default port: 12345
- Message format: "IMU_DATA: Count=X, X=Y, Y=Z, Z=W"

## Testing
The Python simulator provides:
1. Test packet bursts
2. Continuous transmission
3. Malformed packet testing
4. Network response validation