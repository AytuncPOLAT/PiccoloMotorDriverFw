#include "serial_port_unix.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <glob.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>
#include <algorithm>

namespace
{
speed_t toBaudConstant(int baudRate)
{
    switch (baudRate)
    {
        case 9600:
            return B9600;
        case 19200:
            return B19200;
        case 38400:
            return B38400;
        case 57600:
            return B57600;
        case 115200:
            return B115200;
        case 230400:
            return B230400;
#ifdef B460800
        case 460800:
            return B460800;
#endif
#ifdef B921600
        case 921600:
            return B921600;
#endif
        default:
            return static_cast<speed_t>(0);
    }
}

bool writeAllPosix(int fd, const uint8_t* data, std::size_t size, std::string& errorMessage)
{
    std::size_t written = 0;
    while (written < size)
    {
        const ssize_t result = ::write(fd, data + written, size - written);
        if (result < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                pollfd pfd = {fd, POLLOUT, 0};
                const int pollResult = ::poll(&pfd, 1, 100);
                if (pollResult >= 0)
                {
                    continue;
                }
            }

            errorMessage = "Write failed: " + std::string(std::strerror(errno));
            return false;
        }

        written += static_cast<std::size_t>(result);
    }

    return true;
}
}

SerialPortUnix::SerialPortUnix()
    : fd_(-1)
{
}

SerialPortUnix::~SerialPortUnix()
{
    disconnect();
}

std::vector<std::string> SerialPortUnix::listAvailablePorts(int maxPort) const
{
    (void)maxPort;
    std::vector<std::string> ports;
    const char* patterns[] = {
        "/dev/ttyACM*",
        "/dev/ttyUSB*",
        "/dev/ttyS*",
    };

    for (const char* pattern : patterns)
    {
        glob_t globResult = {};
        if (::glob(pattern, 0, nullptr, &globResult) == 0)
        {
            for (std::size_t i = 0; i < globResult.gl_pathc; ++i)
            {
                ports.emplace_back(globResult.gl_pathv[i]);
            }
        }
        ::globfree(&globResult);
    }

    std::sort(ports.begin(), ports.end());
    ports.erase(std::unique(ports.begin(), ports.end()), ports.end());
    return ports;
}

bool SerialPortUnix::connect(const std::string& portName, int baudRate, std::string& errorMessage)
{
    disconnect();

    const std::string resolvedPort = (portName.rfind("/dev/", 0) == 0) ? portName : ("/dev/" + portName);
    const int serialFd = ::open(resolvedPort.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (serialFd < 0)
    {
        errorMessage = "Unable to open " + resolvedPort + ": " + std::string(std::strerror(errno));
        return false;
    }

    termios tty = {};
    if (::tcgetattr(serialFd, &tty) != 0)
    {
        errorMessage = "tcgetattr failed for " + resolvedPort + ": " + std::string(std::strerror(errno));
        ::close(serialFd);
        return false;
    }

    const speed_t baudConst = toBaudConstant(baudRate);
    if (baudConst == static_cast<speed_t>(0))
    {
        errorMessage = "Unsupported baud rate: " + std::to_string(baudRate);
        ::close(serialFd);
        return false;
    }

    ::cfmakeraw(&tty);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
#ifdef CRTSCTS
    tty.c_cflag &= ~CRTSCTS;
#endif
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;

    ::cfsetispeed(&tty, baudConst);
    ::cfsetospeed(&tty, baudConst);

    if (::tcsetattr(serialFd, TCSANOW, &tty) != 0)
    {
        errorMessage = "tcsetattr failed for " + resolvedPort + ": " + std::string(std::strerror(errno));
        ::close(serialFd);
        return false;
    }

    ::tcflush(serialFd, TCIOFLUSH);

    fd_ = serialFd;
    portName_ = resolvedPort;
    return true;
}

void SerialPortUnix::disconnect()
{
    if (fd_ >= 0)
    {
        ::close(fd_);
        fd_ = -1;
    }
    portName_.clear();
}

bool SerialPortUnix::isConnected() const
{
    return fd_ >= 0;
}

std::string SerialPortUnix::connectedPort() const
{
    return portName_;
}

bool SerialPortUnix::write(const uint8_t* data, std::size_t size, std::string& errorMessage)
{
    if (!isConnected())
    {
        errorMessage = "No serial connection.";
        return false;
    }

    return writeAllPosix(fd_, data, size, errorMessage);
}

bool SerialPortUnix::read(uint8_t* buffer, std::size_t maxSize, std::size_t& bytesRead, std::string& errorMessage)
{
    bytesRead = 0;

    if (!isConnected())
    {
        errorMessage = "No serial connection.";
        return false;
    }

    const ssize_t result = ::read(fd_, buffer, maxSize);
    if (result > 0)
    {
        bytesRead = static_cast<std::size_t>(result);
        return true;
    }

    if (result == 0)
    {
        return true;
    }

    if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
    {
        return true;
    }

    errorMessage = "Read failed: " + std::string(std::strerror(errno));
    return false;
}
