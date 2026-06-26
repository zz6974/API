# CSerialPort

[CSerialPort](https://github.com/itas109/CSerialPort) 是一个跨平台的串口通信库，支持多种操作系统和编程语言绑定。它提供了一套统一的API用于串口通信，适用于GUI、TUI和无界面应用程序开发。

## 特性

- 跨平台支持（Windows、Linux、macOS等）
- 支持多种操作模式：同步/异步
- 提供丰富的串口配置选项（波特率、校验位、数据位、停止位等）
- 支持事件驱动的数据接收机制
- 包含多种示例程序（MFC、QT、WXWidgets、TUI、NoGui等）
- 支持C/C++、CSharp、Java、Python、JavaScript等多种语言绑定

## 最新版本

当前版本: 4.3.1.240204 (2024-02-04)

## 快速开始

### 使用vcpkg安装

```bash
vcpkg install cserialport
```

### 交叉编译

支持多种架构的交叉编译，包含CMake工具链配置文件：
- aarch64
- arm
- mips64el
- riscv64

## 示例代码

### C++ 基础使用

```cpp
#include "CSerialPort/SerialPort.h"

class MyListener : public itas109::CSerialPortListener
{
public:
    MyListener(itas109::CSerialPort *sp) : p_sp(sp) {}
    
    void onReadEvent(const char *portName, unsigned int readBufferLen)
    {
        // 处理接收到的数据
        char data[1024] = {0};
        int bytesRead = p_sp->readData(data, sizeof(data));
        // ...
    }

private:
    itas109::CSerialPort *p_sp;
};

int main()
{
    // 创建串口对象
    itas109::CSerialPort sp;
    
    // 初始化串口
    sp.init("/dev/ttyUSB0", itas109::BaudRate::BaudRate9600);
    
    // 设置操作模式为异步模式
    sp.setOperateMode(itas109::AsynchronousOperate);
    
    // 创建监听器
    MyListener listener(&sp);
    sp.connectReadEvent(&listener);
    
    // 打开串口
    if(sp.open())
    {
        // 串口操作...
        
        // 关闭串口
        sp.close();
    }
    
    return 0;
}
```

## 文档

- [中文文档](doc/CSerialPort_doc_cn.chm)
- [英文文档](doc/CSerialPort_doc_en.chm)

## 应用案例

1. [CommMaster通信大师](https://gitee.com/itas109/CommMaster)
2. [CommLite](https://github.com/itas109/CommLite)

## 许可证

本项目采用开源协议，请查看[LICENSE](LICENSE)文件获取详细信息。

## 联系方式

如需技术支持或商务合作，请通过GitHub/Gitee页面联系作者。

## 开发环境要求

- 支持C++11的编译器
- CMake 3.14+
- 支持跨语言绑定的工具（SWIG等）

## 构建说明

请参考各绑定目录下的README文件获取具体构建方法：
- [C绑定](bindings/c/README.md)
- [CSharp绑定](bindings/csharp/README.md)
- [Java绑定](bindings/java/README.md)
- [JavaScript绑定](bindings/javascript/README.md)
- [Python绑定](bindings/python/README.md)

## 贡献指南

欢迎贡献代码和文档，提交PR前请确保：
1. 遵循代码风格
2. 包含必要的测试
3. 更新相关文档

## 致谢

感谢所有贡献者和使用者的支持！