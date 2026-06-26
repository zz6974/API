#ifndef IMGUI_CCSERIALPORT_H
#define IMGUI_CCSERIALPORT_H

// 根据操作系统定义睡眠函数，Windows使用Sleep，其他平台使用usleep
#ifdef _WIN32
#include <windows.h>
#define imsleep(microsecond) Sleep(microsecond) // ms
#else
#include <unistd.h>
#define imsleep(microsecond) usleep(1000 * microsecond) // ms
#endif

// 包含必要的头文件

#include <functional>
#include <string>

#include "CSerialPort/SerialPort.h"
#include "CSerialPort/SerialPortInfo.h"

// 使用itas109命名空间
using namespace itas109;

// 定义CTL命名空间
namespace CTL {

    /**
     * @class SerialPort
     * @brief 串口通信类，继承自CSerialPortListener，用于管理串口设备的打开、关闭、读写等操作。
     */
    class SerialPort : public CSerialPortListener
    {
    public:
        /**
         * @brief 默认构造函数，初始化SerialPort对象。
         */
        SerialPort();

        /**
         * @brief 带参数的构造函数，使用已有的CSerialPort对象进行初始化。
         *
         * @param sp 已经创建好的CSerialPort对象指针。
         */
        explicit SerialPort(CSerialPort *sp) : COM(sp)
        {
            // 获取所有可用的串口信息并存储在PortsList中
            PortsList = CSerialPortInfo::availablePortInfos();
        };

        /**
         * @brief 获取当前系统中可用串口的数量。
         *
         * @return unsigned int 可用串口的数量。
         */
        unsigned int GetPortSize();

        /**
         * @brief 获取所有可用串口的详细信息。
         *
         * @return SerialPortInfo* 指向包含所有串口信息的数组指针。
         */
        std::vector<SerialPortInfo> GetProtInfoAll();

        /**
         * @brief 初始化串口参数。
         *
         * @param portName 串口号（例如："COM1"）。
         * @param baudRate 波特率，默认为9600。
         * @param parity 校验位，默认为无校验。
         * @param dataBits 数据位，默认为8位。
         * @param stopbits 停止位，默认为1位。
         * @param flowControl 流控，默认为无流控。
         * @param readBufferSize 读缓冲区大小，默认为4096字节。
         */
        void Initialize(
            const char *portName,
            int baudRate = itas109::BaudRate::BaudRate9600,
            itas109::Parity parity = itas109::Parity::ParityNone,
            itas109::DataBits dataBits = itas109::DataBits::DataBits8,
            itas109::StopBits stopbits = itas109::StopBits::StopOne,
            itas109::FlowControl flowControl = itas109::FlowControl::FlowNone,
            unsigned int readBufferSize = 4096);

        /**
         * @brief 设置串口读取间隔时间。
         *
         * @param ms 读取间隔时间，单位为毫秒。
         */
        void SetReadIntervalTime(unsigned int ms);

        /**
         * @brief 打开串口。
         *
         * @return bool 如果成功打开串口返回true，否则返回false。
         */
        bool Open();

        /**
         * @brief 检查串口是否已经打开。
         *
         * @return bool 如果串口已打开返回true，否则返回false。
         */
        bool IsOpen();

        /**
         * @brief 获取上次操作的错误码。
         *
         * @return int 错误码。
         */
        int GetLastError();

        /**
         * @brief 获取上次操作的错误信息。
         *
         * @return const char* 错误信息字符串。
         */
        const char *GetLastErrorMsg();

        /**
         * @brief 处理串口读取事件，当有数据可读时调用此方法。
         *
         * @param portName 串口号。
         * @param readBufferLen 可读数据的长度。
         */
        void onReadEvent(const char *portName, unsigned int readBufferLen) override;

        /**
         * @brief 向串口写入数据。
         *
         * @param data 要写入的数据指针。
         * @param size 数据大小，以字节为单位。
         */
        void Write(const void *data, int size);

        /**
         * @brief 向串口写入字符串数据。
         *
         * @param data 要写入的字符串。
         */
        void Write(std::string data);

        /**
         * @brief 获取从串口读取的数据。
         *
         * @return char* 读取到的数据指针。
         */
        char *GetReadData();

        /**
         * @brief 关闭串口。
         */
        void Close();

        /**
         * @brief 设置读取回调函数，当有数据可读时调用此回调函数。
         *
         * @tparam Func 回调函数类型。
         * @tparam Args 回调函数参数类型。
         * @param fun 回调函数。
         * @param args 回调函数参数。
         * @return bool 如果设置成功返回true，否则返回false。
         */
        template <typename Func, typename... Args>
        bool SetReadClick(Func &&fun, Args &&... args);

    private:
        /**
         * @brief 指向实际串口对象的指针。
         */
        CSerialPort *COM;

        /**
         * @brief 存储所有可用串口信息的列表。
         */
        std::vector<SerialPortInfo> PortsList;

        /**
         * @brief 回调函数，用于处理读取事件。
         */
        std::function<void()> Function;

        /**
         * @brief 读取到的数据缓冲区。
         */
        char *Date;
    };
}

/**
 * @brief 设置读取回调函数的实现。
 *
 * @tparam Func 回调函数类型。
 * @tparam Args 回调函数参数类型。
 * @param fun 回调函数。
 * @param args 回调函数参数。
 * @return bool 如果设置成功返回true，否则返回false。
 */
template<typename Func, typename... Args>
inline bool CTL::SerialPort::SetReadClick(Func &&fun, Args &&... args)
{
    Function = std::bind(std::forward<Func>(fun), std::forward<Args>(args)...);
    return true;
}

#endif
