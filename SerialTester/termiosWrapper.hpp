#ifndef TERMIOSWRAPPER_H
#define TERMIOSWRAPPER_H

#include <termios.h>
#include <cstring>
#include <fcntl.h>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <stdint.h>

class TermiosWrapper
{
public:
    TermiosWrapper();

    TermiosWrapper(const TermiosWrapper&) = delete;
    TermiosWrapper& operator=(const TermiosWrapper&) = delete;

    bool Open(char *serialDev);
    void Init();
    bool Close();
    bool IsConnected();

    int Transmit(uint8_t *ch, uint32_t size);
    int Receive(uint8_t *data, uint32_t size);

    uint32_t ReadRxBuffer(uint8_t *data);

private:
    int fd = 0;
    bool isConnected = false;
    struct termios tio;
    struct termios old_tio;

    uint8_t rxBuffer[256];
    uint8_t rxByteIndex = 0;
    uint8_t byteIndexOld = 0;
};

#endif // TERMIOSWRAPPER_H
