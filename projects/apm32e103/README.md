# CherryDAP for APM32E103

此工程为 APM32E103 微控制器移植的 CherryDAP 调试器固件。

## 硬件规格

- **MCU**: APM32E103CE (512KB Flash, 128KB RAM @ 120MHz)
- **USB**: Full Speed Device (CherryUSB fsdev port)
- **Bootloader**: 兼容 BlackMagic DFU bootloader
- **应用起始地址**: 0x08002000 (8KB bootloader空间)

## 引脚定义

基于 BlackMagic Native Plus 的引脚定义：

### JTAG/SWD 接口
- **TCK/SWCLK**: PA5
- **TMS/SWDIO**: PA4
- **TDI**: PA3 (仅JTAG)
- **TDO**: PA6 (仅JTAG)
- **nTRST**: PC13
- **nRESET**: PA2

### LED 指示灯
- **LED_RUNNING**: PB2 (黄色LED)
- **LED_IDLE**: PB10 (橙色LED)
- **LED_ERROR**: PB11 (红色LED)

### USB
- **USB_D+**: PA12
- **USB_D-**: PA11
- **USB_PU**: PA8 (上拉控制)

### UART (USB2UART功能)
- **UART_TX**: PA9 (USART1)
- **UART_RX**: PA10 (USART1)

## 编译

```bash
cd projects/apm32e103
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

编译成功后会生成：
- `apm32e103_cherrydap.elf` - ELF格式固件
- `apm32e103_cherrydap.bin` - 二进制固件
- `apm32e103_cherrydap.hex` - HEX格式固件

## 烧录

### 使用 DFU 模式
如果已刷入 BlackMagic DFU bootloader:
```bash
dfu-util -a 0 -s 0x08002000 -D apm32e103_cherrydap.bin
```

### 使用 DFU Runtime 从 APP 触发复位
APP 已支持 DFU Runtime `DETACH` 请求，行为与 BlackMagic `dfu_detach` 一致：收到 detach 后立即系统复位。

示例：
```bash
dfu-util -e
```

复位后由 bootloader 自身逻辑决定进入 DFU 或跳转 APP。

### 使用 OpenOCD
```bash
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
  -c "program apm32e103_cherrydap.elf verify reset exit"
```

## 功能特性

- ✅ **CMSIS-DAP v1/v2**: 完整的调试协议支持
- ✅ **SWD**: Serial Wire Debug 接口
- ✅ **JTAG**: JTAG调试接口 (支持最多8个设备)
- ✅ **USB CDC**: USB转串口功能
- ✅ **DFU Runtime**: 支持运行时 detach 复位
- ✅ **目标板复位**: 硬件复位控制
- ✅ **LED指示**: 运行状态/连接状态/错误状态
- ⚠️ **SWO**: 暂未实现 (设置为0)

## 配置参数

- **CPU频率**: 120MHz
- **默认SWD/JTAG频率**: 10MHz
- **USB包大小**: 64字节 (Full Speed)
- **USB VID/PID**: 0x0D28/0x0204 (ARM标准CMSIS-DAP)

## 目录结构

```
apm32e103/
├── CMakeLists.txt          # CMake构建文件
├── apm32e103_app.ld        # 链接脚本 (APP起始于0x08002000)
├── Drivers/                # APM32 SDK驱动
│   ├── CMSIS/             # CMSIS核心文件
│   ├── Device/            # APM32E10x设备文件
│   └── StdPeriphDriver/   # 标准外设驱动库
├── Inc/                    # 头文件
│   ├── board.h            # 板级配置
│   ├── DAP_config.h       # DAP配置(GPIO定义)
│   ├── usb_config.h       # CherryUSB配置
│   └── apm32e10x_int.h    # 中断处理
└── Src/                    # 源文件
    ├── main.c             # 主程序
    ├── board.c            # 板级支持
    └── apm32e10x_int.c    # 中断服务函数
```

## 待完成功能

- [ ] USB转串口实际实现
- [ ] SWO追踪功能
- [ ] HID/MSC支持(可选)
- [ ] 性能优化和测试

## 注意事项

1. 此固件需要先刷入 BlackMagic DFU bootloader 才能通过 DFU 升级
2. 首次烧录需使用 SWD/JTAG 调试器
3. 固件编译时使用 `-O3` 优化级别以获得最佳性能
4. 启动文件已配置向量表偏移 (VECT_TAB_OFFSET = 0x2000)

## 已知问题

1. USB2UART 在“打开串口并切换目标供电状态”的边界场景下，仍可能偶发少量 `0x00` 或乱码首包。
2. 当前已采用“目标电压稳定后再启用 RX 路径”的策略，但在少数组合下仍可能出现瞬态噪声，后续继续优化。

## 参考资源

- [CherryDAP 项目](https://github.com/CherryUSB/CherryDAP)
- [CherryUSB](https://github.com/cherry-embedded/CherryUSB)
- [BlackMagic Probe](https://github.com/blackmagic-debug/blackmagic)
- [APM32E10x SDK](https://www.geehy.com/)

## 许可证

遵循 CherryDAP 和 CherryUSB 的开源许可证。
