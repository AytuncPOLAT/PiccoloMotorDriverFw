#include <iostream>
#include <cstring>
#include <chrono>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include "LinkLayer.hpp"
#include "Crc16.hpp"

int main(int argc, char *argv[]) 
{
    if (argc != 8) 
    {
        std::cerr << "Usage: " << argv[0] << " <serial_device> <cmd> <address> <data0> <data1> <data2> <data3>" << std::endl;
        return 1;
    }

    char *serialDev = argv[1];
    int cmdInt = std::stoi(argv[2]);
    uint8_t address = std::stoi(argv[3]);
    uint32_t data0 = std::stoul(argv[4]);
    uint32_t data1 = std::stoul(argv[5]);
    uint32_t data2 = std::stoul(argv[6]);
    uint32_t data3 = std::stoul(argv[7]);

    Common::DataFrame df;
    df.cmd = static_cast<Common::CMD_TYPE>(cmdInt);
    df.address = address;
    df.data0 = data0;
    df.data1 = data1;
    df.data2 = data2;
    df.data3 = data3;

    Common::Crc16 crc;
    uint16_t checksum = crc.Calculate(0, reinterpret_cast<uint8_t*>(&df), sizeof(df) - 2);
    df.checksum = checksum;

    int fd = open(serialDev, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        std::cerr << "Failed to open serial device: " << serialDev << std::endl;
        return 1;
    }

    struct termios old_tio;
    tcgetattr(fd, &old_tio);

    struct termios tio;
    memset(&tio, 0, sizeof(tio));
    tio.c_iflag = 0;
    tio.c_oflag = 0;
    tio.c_cflag = CS8 | CREAD | CLOCAL;
    tio.c_lflag = 0;
    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = 0;
    cfsetospeed(&tio, B115200);
    cfsetispeed(&tio, B115200);
    tcsetattr(fd, TCSANOW, &tio);
    fcntl(fd, F_SETFL, 0);

    int sent = write(fd, &df, sizeof(df));
    if (sent != sizeof(df)) {
        std::cerr << "Failed to send data" << std::endl;
        tcsetattr(fd, TCSANOW, &old_tio);
        close(fd);
        return 1;
    }
    
    std::cout << "Data sent successfully" << std::endl;

    // Listen for response for 1 second
    auto start = std::chrono::high_resolution_clock::now();
    while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start).count() < 1) {
        uint8_t buffer[64];
        int len = read(fd, buffer, sizeof(buffer));
        if (len == sizeof(Common::DataFrame)) {
            Common::DataFrame *received = reinterpret_cast<Common::DataFrame*>(buffer);
            std::cout << "Received: cmd=" << static_cast<int>(received->cmd)
                      << " address=" << static_cast<int>(received->address)
                      << " data0=" << received->data0
                      << " data1=" << received->data1
                      << " data2=" << received->data2
                      << " data3=" << received->data3
                      << " checksum=" << received->checksum << std::endl;
        }
        usleep(10000); // 10ms sleep
    }

    tcsetattr(fd, TCSANOW, &old_tio);
    close(fd);
    return 0;
}