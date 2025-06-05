#include "imu_parser_api.h"


int main(int argc, char* argv[]) {
    // Create a parser oobject
    IMUParser parser;
    
    // Default to /dev/tty1 as specified in requirements
    // but allow override for testing purposes
    const char* uart_device = "/dev/tty1";
    if (argc > 1) {
        uart_device = argv[1];
        std::cout << "Using custom UART device: " << uart_device << std::endl;
    } else {
        std::cout << "Using default UART device as specified: " << uart_device << std::endl;
        std::cout << "Note: /dev/tty1 may require sudo privileges" << std::endl;
        std::cout << "For testing, run with: ./imu_parser <device_path>" << std::endl;
    }
    
    // Output error to prompt user what virtual ports they should try to use if there
    // was an issue crreating a valid terminal device.
    if (!parser.initUART(uart_device)) {
        std::cerr << "Failed to initialize UART on " << uart_device << std::endl;
        if (strcmp(uart_device, "/dev/tty1") == 0) {
            std::cerr << "Note: /dev/tty1 requires root privileges. Try:" << std::endl;
            std::cerr << "  sudo ./build/imu_parser" << std::endl;
            std::cerr << "Or for testing with virtual ports:" << std::endl;
            std::cerr << "  ./build/imu_parser /dev/ttys006  # macOS" << std::endl;
            std::cerr << "  ./build/imu_parser /dev/pts/2    # Linux" << std::endl;
        }
        return -1;
    }
    
    // Throw error if a socket couldn't be created.
    if (!parser.initBroadcast()) {
        std::cerr << "Failed to initialize broadcast" << std::endl;
        return -1;
    }
   
    // If there were no errors creating a virtual port or a socket, output that processor has started.
    std::cout << "IMU Parser started on " << uart_device << ", processing every 80ms..." << std::endl;
    
    // Main loop - execute every 80ms as specified in requirements
    while (true) {
        auto start_time = std::chrono::steady_clock::now();
        
        // Make the call to process everything.
        parser.processIMUData();
        
        // Sleep for remaining time to maintain 80ms cycle
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        auto sleep_time = std::chrono::milliseconds(80) - elapsed;
        
        // Put the program into standby if completed before next cycle.
        if (sleep_time > std::chrono::milliseconds(0)) {
            std::this_thread::sleep_for(sleep_time);
        }
    }
    
    return 0;
}