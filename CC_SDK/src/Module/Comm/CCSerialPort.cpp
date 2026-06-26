#include "CCSerialPort.h"

CTL::SerialPort::SerialPort()
{
    this->COM = new CSerialPort();
    PortsList = CSerialPortInfo::availablePortInfos();
}

unsigned int CTL::SerialPort::GetPortSize()
{
    return (int)PortsList.size();
}

std::vector<SerialPortInfo> CTL::SerialPort::GetProtInfoAll()
{
    std::vector<SerialPortInfo> serialPortInfo;
    const int d = this->GetPortSize();
    for (int i = 0; i < d; ++i){
        serialPortInfo.push_back(PortsList[i]);
        // printf("%d - %s\n", i, serialPortInfo[i].portName);
    }
    return serialPortInfo;
}

void CTL::SerialPort::Initialize(const char *portName, int baudRate, itas109::Parity parity, itas109::DataBits dataBits,
                              itas109::StopBits stopbits, itas109::FlowControl flowControl,
                              unsigned int readBufferSize)
{
    this->COM->init(portName, baudRate, parity, dataBits, stopbits,flowControl,readBufferSize);
}

void CTL::SerialPort::SetReadIntervalTime(unsigned int ms)
{
    this->COM->setReadIntervalTimeout(ms);
}

bool CTL::SerialPort::Open()
{
    auto f = this->COM->open();
    this->COM->connectReadEvent(this);
    return f;
}

bool CTL::SerialPort::IsOpen()
{
    return this->COM->isOpen();
}

int CTL::SerialPort::GetLastError()
{
    return this->COM->getLastError();
}

const char *CTL::SerialPort::GetLastErrorMsg()
{
    return this->COM->getLastErrorMsg();
}

void CTL::SerialPort::onReadEvent(const char *portName, unsigned int readBufferLen)
{
    if (readBufferLen > 0)
    {
        char *data = new char[readBufferLen + 1];
        if(data)
        {
            int recLen = COM->readData(data, readBufferLen);
            if (recLen > 0)
            {
                data[recLen] = '\0';
                Date = data;
                if(Function)
                {
                    Function();
                }

            }

            delete[] data;
            data = NULL;
        }

    }

}

void CTL::SerialPort::Write(const void *data, int size)
{
    this->COM->writeData(data, size);
}

void CTL::SerialPort::Write(std::string data)
{
    this->COM->writeData(data.c_str(), data.length());
}

char *CTL::SerialPort::GetReadData()
{
    return Date;
}

void CTL::SerialPort::Close()
{
    this->COM->close();
}


