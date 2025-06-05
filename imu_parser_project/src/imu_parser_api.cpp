#include "imu_parser_api.h"

// Define the start frame bytes
const uint8_t IMUParser::START_FRAME[4] = {0x7F, 0xF0, 0x1C, 0xAF};

    // Struct constructor for IMUData stores the packet count and the regional predictor coordinates
    IMUData::IMUData() : packet_count(0), x_rate_rdps(0.0f), y_rate_rdps(0.0f), z_rate_rdps(0.0f) {}

    // Class constructor for IMUPasrser
    // uart_fd: Initialize file descriptor to -1 so default is that no UART device found.
    // buffer: Initialize the buuffer to specified size (4096 bytes).
    // buffer_pos: Initialize the buffer position to 0.
    // broadcast_socket: Initialize socket to -1 so default is that no socket is found.
    IMUParser::IMUParser() : uart_fd(-1), buffer(BUFFER_SIZE), buffer_pos(0), broadcast_socket(-1) {}
    
    // Class destructor for IMUParser
    // uart_fd: Close file descriptor if open.
    // broadcast_socket: Close socket if open.
    IMUParser::~IMUParser() {
        if (uart_fd >= 0) close(uart_fd);
        if (broadcast_socket >= 0) close(broadcast_socket);
    }
    
    // Setup UART by checking file descriptor, creating valid terminal device,
    // Setting baud rate,configuring tty flags, and validating attributes.
    bool IMUParser::initUART(const char* device) {
        // Check to see if file descriptor is available to be opened with the
        // availabilites for read, write, terminal flexibility, and no delays.
        // Throw an error and return false if not.
        uart_fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (uart_fd < 0) {
            std::cerr << "Failed to open UART device: " << device << std::endl;
            return false;
        }
        
        // Create a terminal device to handle I/O.
        struct termios tty;
        // Test to see if terminal parameters could be fetched. Throw an error
        // and return false if not.
        if (tcgetattr(uart_fd, &tty) != 0) {
            std::cerr << "Error getting UART attributes" << std::endl;
            return false;
        }
        
        // Configure for 921600 baud, 8N1
        // Mac doesn't have B921600, use custom baud rate setting
        #ifdef __APPLE__
            cfsetspeed(&tty, 921600);
        #else
        // Equivalent command in linux
            cfsetospeed(&tty, B921600);
            cfsetispeed(&tty, B921600);
        #endif
        

        // Configure tty flags to ensure proper communication
        tty.c_cflag &= ~PARENB; // No parity
        tty.c_cflag &= ~CSTOPB; // One stop bit
        tty.c_cflag &= ~CSIZE;  // Clear data size bits
        tty.c_cflag |= CS8;     // 8 data bits
        tty.c_cflag &= ~CRTSCTS; // No hardware flow control
        tty.c_cflag |= CREAD | CLOCAL; // Enable reading
        
        tty.c_lflag &= ~ICANON; // Raw mode
        tty.c_lflag &= ~ECHO;   // No echo
        tty.c_lflag &= ~ECHOE; // No erasure of input
        tty.c_lflag &= ~ISIG; // Prevents interpretting special characterr interrupts
        
        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // No software flow control.

        // Don't ignore break condition, signal interrupt on break, mark parity, 
        // strip character, mapping NL to CR and vice-versa, CR, and NL
        tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);
        
        tty.c_oflag &= ~OPOST; // Raw output
        
        // If the attributes change, its not standard input, and parameters cannot 
        // be set, throw an error and return false.
        if (tcsetattr(uart_fd, TCSANOW, &tty) != 0) {
            std::cerr << "Error setting UART attributes" << std::endl;
            return false;
        }
        
        return true;
    }

    // Initialize broadcast by creating, enabling, and configuring a socket.
    bool IMUParser::initBroadcast(int port) {
        // Open socket with protocols.
        broadcast_socket = socket(AF_INET, SOCK_DGRAM, 0);
        // If unable to make the socket, throw an errorr and return false.
        if (broadcast_socket < 0) {
            std::cerr << "Failed to create broadcast socket" << std::endl;
            return false;
        }
        
        // Set the enable flag to true.
        int broadcast_enable = 1;
        // If the socket is unable to be broadcasted, throw an error and return false.
        if (setsockopt(broadcast_socket, SOL_SOCKET, SO_BROADCAST, 
                      &broadcast_enable, sizeof(broadcast_enable)) < 0) {
            std::cerr << "Failed to enable broadcast" << std::endl;
            return false;
        }
        
        // Allocate memory to the socket and configure ports appropriately.
        memset(&broadcast_addr, 0, sizeof(broadcast_addr));
        broadcast_addr.sin_family = AF_INET;
        broadcast_addr.sin_addr.s_addr = inet_addr("127.255.255.255"); // localhost broadcast
        broadcast_addr.sin_port = htons(port); // Store numbers in memory in network byte order
        
        return true;
    }
    
    // Reads UART data by slicing the main buffer into smaller chuncks (fourths)
    // to help assist the program to keep up with the fast baud rate.
    void IMUParser::readUARTData() {
        // Create temporary buffer.
        uint8_t temp_buffer[1024];
        // Store data from the file descriptor into the temporary buffer.
        ssize_t bytes_read = read(uart_fd, temp_buffer, sizeof(temp_buffer));
        
        // Only add to circular buffer if any data was obtained from the file descriptor.
        if (bytes_read > 0) {
            // Add to circular buffer
            for (ssize_t i = 0; i < bytes_read; i++) {
                buffer[buffer_pos] = temp_buffer[i];
                buffer_pos = (buffer_pos + 1) % BUFFER_SIZE;
            }
        }
    }
    
    // Function to locate the start frame of the packet.
    bool IMUParser::findStartFrame(size_t& frame_start) {
        // Look for the 4-byte start frame: 0x7F, 0xF0, 0x1C, 0xAF
        for (size_t i = 0; i < BUFFER_SIZE - 3; i++) {
            size_t pos = (buffer_pos + i) % BUFFER_SIZE;
            
            // If frame start found, set it to the designated variable and return true.
            if (buffer[pos] == START_FRAME[0] &&
                buffer[(pos + 1) % BUFFER_SIZE] == START_FRAME[1] &&
                buffer[(pos + 2) % BUFFER_SIZE] == START_FRAME[2] &&
                buffer[(pos + 3) % BUFFER_SIZE] == START_FRAME[3]) {
                frame_start = pos;
                return true;
            }
        }
        // Otherwise return false.
        return false;
    }
    
    // Convert from network byte order (big-endian) to host byte order
    uint32_t IMUParser::networkToHost32(const uint8_t* data) {
        return ntohl(*reinterpret_cast<const uint32_t*>(data));
    }
    
    // Same as above but returns a float instead of a uint32_t
    float IMUParser::networkToHostFloat(const uint8_t* data) {
        uint32_t temp = ntohl(*reinterpret_cast<const uint32_t*>(data));
        return *reinterpret_cast<const float*>(&temp);
    }
    
    // Function for parsing packet. Start at the frame offset and add to the IMUData Struct.
    bool IMUParser::parsePacket(size_t frame_start, IMUData& data) {
        // Extract packet data from circular buffer
        uint8_t packet_data[PACKET_SIZE];
        for (int i = 0; i < PACKET_SIZE; i++) {
            packet_data[i] = buffer[(frame_start + i) % BUFFER_SIZE];
        }
        
        // Skip the 4-byte start frame and parse the payload
        data.packet_count = networkToHost32(&packet_data[4]);
        data.x_rate_rdps = networkToHostFloat(&packet_data[8]);
        data.y_rate_rdps = networkToHostFloat(&packet_data[12]);
        data.z_rate_rdps = networkToHostFloat(&packet_data[16]);
        
        return true;
    }
    
    // Broadcasts the information to the local host to validate the output.
    void IMUParser::broadcastData(const IMUData& data) {
        // Create the message.
        char message[256];
        snprintf(message, sizeof(message), 
                "IMU_DATA: Count=%u, X=%.6f, Y=%.6f, Z=%.6f\n",
                data.packet_count, data.x_rate_rdps, data.y_rate_rdps, data.z_rate_rdps);
        
        //Broadcast it        
        sendto(broadcast_socket, message, strlen(message), 0,
               (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));
    }

    // Function that:
    // 1) Reads the UART data by leveraging a temporary buffer
    // 2) Verifies the start frame
    // 3) Verifies the packet data
    // 4) prints the message
    // 5) broadcasts the message to the local host
    // 6) Resets the buffer to the next packet
    void IMUParser::processIMUData() {
        readUARTData();
        
        size_t frame_start;
        if (findStartFrame(frame_start)) {
            IMUData data;
            if (parsePacket(frame_start, data)) {
                std::cout << "Parsed IMU Data - Count: " << data.packet_count 
                         << ", X: " << data.x_rate_rdps 
                         << ", Y: " << data.y_rate_rdps 
                         << ", Z: " << data.z_rate_rdps << std::endl;
                
                broadcastData(data);
                
                // Move buffer position past this packet
                buffer_pos = (frame_start + PACKET_SIZE) % BUFFER_SIZE;
            }
        }
    }
