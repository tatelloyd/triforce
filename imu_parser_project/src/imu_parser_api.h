// Header Guards
#ifndef MY_IMU_PARSER_API_H
#define MY_IMU_PARSER_API_H

// included libraries to provide full functionality
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <chrono>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

// Struct that will hold the parsed data from the byte stream.
// Constructor added to facilitate
struct IMUData {
    uint32_t packet_count;
    float x_rate_rdps;
    float y_rate_rdps;
    float z_rate_rdps;
    
    IMUData();
};

// Class holding all the functions and variable to use the parser.
// Variables and MACROS are private as a security design.
// Functions are public for easy access.
class IMUParser {
private:
    // Length of Packet as specified by the problem set
    static const size_t PACKET_SIZE = 20;
    // Large buffer size to bridge high baud rate and low processing time.
    static const size_t BUFFER_SIZE = 4096; 
    // Arrray to help parse the start frame of the packet
    static const uint8_t START_FRAME[4];
    
    // File descriptor
    int uart_fd;
    std::vector<uint8_t> buffer;
    size_t buffer_pos;
    
    // Network broadcast socket
    int broadcast_socket;
    struct sockaddr_in broadcast_addr;
    
public:
    // Constructor and destructor
    IMUParser();
    ~IMUParser();
    
    bool initUART(const char* device = "/dev/tty1");
    
    bool initBroadcast(int port = 12345);
    
    void readUARTData();
    
    bool findStartFrame(size_t& frame_start);
    
    // Convert from network byte order (big-endian) to host byte order
    uint32_t networkToHost32(const uint8_t* data);
    
    // Same as above but returns a float instead of a uint32_t
    float networkToHostFloat(const uint8_t* data);
    
    bool parsePacket(size_t frame_start, IMUData& data);
    
    void broadcastData(const IMUData& data);
    
    void processIMUData();
};

#endif