#!/usr/bin/env python3
import struct
import time
import sys
import os
import math
import argparse
import threading
from typing import Optional

# Create a class to hold all of the methods necessary
class FileBasedIMUSimulator:
    # Initializes a pipe to hold data dumps.
    def __init__(self, output_file: str = "/tmp/imu_data"):
        # Set the default file, the simulator to not be ready,
        # and the packet to be zero.
        self.output_file = output_file
        self.running = False
        self.packet_count = 0
        
        # Start of frame bytes
        self.SOF = bytes([0x7F, 0xF0, 0x1C, 0xAF])
        
        # Create named pipe if it doesn't exist
        self._setup_output()
        
    def _setup_output(self):
        """Setup the output file/pipe"""
        try:
            # Remove existing file/pipe
            if os.path.exists(self.output_file):
                os.remove(self.output_file)
            
            # Create named pipe (FIFO)
            os.mkfifo(self.output_file)
            print(f"Created named pipe: {self.output_file}")
            print("The C++ parser should read from this file.")
        except Exception as e:
            print(f"Note: Could not create named pipe ({e}), using regular file")
            # Will use regular file instead
    
    def create_imu_packet(self, x_rate: float = 0.0, y_rate: float = 0.0, z_rate: float = 0.0) -> bytes:
        """Create an IMU packet according to the specification"""
        packet = bytearray()
        
        # Start of frame (4 bytes)
        packet.extend(self.SOF)
        
        # Packet count (4 bytes, unsigned integer, network byte order)
        packet.extend(struct.pack('!I', self.packet_count))
        
        # Gyro rates (3 x 4 bytes each, IEEE-754 float, network byte order)
        packet.extend(struct.pack('!f', x_rate))
        packet.extend(struct.pack('!f', y_rate))
        packet.extend(struct.pack('!f', z_rate))
        
        # Augment packet count for next packet
        self.packet_count += 1
        return bytes(packet)
    
    def generate_test_data(self) -> tuple:
        """Generate test data for IMU simulation"""
        # Generate sinusoidal test patterns
        t = time.time()
        x_rate = 10.0 * math.sin(t * 0.5)  # Slow sine wave
        y_rate = 5.0 * math.cos(t * 0.3)   # Different frequency cosine
        z_rate = 2.0 * math.sin(t * 0.7)   # Another frequency
        
        return x_rate, y_rate, z_rate
    
    def send_packet(self):
        """Send a single IMU packet"""
        x_rate, y_rate, z_rate = self.generate_test_data()
        packet = self.create_imu_packet(x_rate, y_rate, z_rate)
        
        try:
            # Open file for each write (required for named pipes)
            with open(self.output_file, 'wb') as f:
                f.write(packet)
                f.flush()
            
            print(f"Sent packet {self.packet_count-1}: X={x_rate:.3f}, Y={y_rate:.3f}, Z={z_rate:.3f}")
            return True
        except Exception as e:
            print(f"Error sending packet: {e}")
            return False
    
    def run_continuous(self, frequency_hz: float = 12.5):
        """Run the simulator continuously at specified frequency"""
        period = 1.0 / frequency_hz
        print(f"Starting IMU simulation at {frequency_hz} Hz (period: {period:.3f}s)")
        print(f"Writing to: {self.output_file}")
        print("Press Ctrl+C to stop...")
        
        self.running = True
        try:
            while self.running:
                start_time = time.time()
                
                if not self.send_packet():
                    break
                
                # Sleep for the remainder of the period
                elapsed = time.time() - start_time
                sleep_time = max(0, period - elapsed)
                time.sleep(sleep_time)
                
        except KeyboardInterrupt:
            print("\nStopping simulator...")
        except BrokenPipeError:
            print("Reader disconnected, stopping...")
        finally:
            self.stop()
    
    def stop(self):
        """Stop the simulator"""
        self.running = False
        print("Simulator stopped.")

def main():
    parser = argparse.ArgumentParser(description='File-Based IMU Data Simulator')
    parser.add_argument('--output', '-o', type=str, default='/tmp/imu_data', help='Output file/pipe path')
    parser.add_argument('--frequency', '-f', type=float, default=12.5, help='Transmission frequency in Hz (default: 12.5)')
    parser.add_argument('--single', '-s', action='store_true', help='Send single packet and exit')
    
    args = parser.parse_args()
    
    try:
        # Create simulator object.
        simulator = FileBasedIMUSimulator(output_file=args.output)
        
        # Send packet if single argument. Otherwise, run the packet at the specified frequency.
        if args.single:
            simulator.send_packet()
        else:
            simulator.run_continuous(args.frequency)

    # Keyboard/Comprehensive error handling        
    except KeyboardInterrupt:
        print("\nExiting...")
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()