#include "termiosWrapper.hpp"

TermiosWrapper::TermiosWrapper()
{}

void TermiosWrapper::Init()
{
    memset(&tio,0,sizeof(tio));
    tio.c_iflag=0;
    tio.c_oflag=0;
    tio.c_cflag=CS8|CREAD|CLOCAL;           // 8n1, see termios.h for more information
    tio.c_lflag=0;
    tio.c_cc[VMIN]=0;
    tio.c_cc[VTIME]=0;
    cfsetospeed(&tio,B115200);
    cfsetispeed(&tio,B115200);
    tcsetattr(fd, TCSANOW, &tio);
}

bool TermiosWrapper::Open(char *serialDev)
{
    fd = open(serialDev, O_RDWR | O_NOCTTY | O_NDELAY);

    if (fd == -1)
    {
        isConnected = false;
        //perror("serialPort: Unable to open - ");
        return false;
    }
    else
    {
        tcgetattr(fd, &old_tio);
        Init();
        isConnected = true;
        fcntl(fd, F_SETFL, 0);
        return true;
    }
    return true;
}

bool TermiosWrapper::Close()
{
    tcsetattr(fd, TCSANOW, &old_tio);
    close(fd);
    isConnected = false;
    return false;
}

bool TermiosWrapper::IsConnected()
{
    return isConnected;
}

int TermiosWrapper::Transmit(uint8_t *ch, uint32_t size)
{
    int error = 0;
    error = write(fd, ch, size);
    if (error < 0) fputs("Serial port error\n", stderr);
    return error;
}

int TermiosWrapper::Receive(uint8_t *data, uint32_t size)
{
    std::memcpy(data, &rxBuffer, size);
    return 0;
}

uint32_t TermiosWrapper::ReadRxBuffer(uint8_t *data)
{
    int len = 0;
    len = read(fd, data, 64);

    return len;
}
