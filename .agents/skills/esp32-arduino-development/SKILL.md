---
name: esp32-arduino-development
description: |
  ESP32 Arduino框架完整开发指南。涵盖Arduino CLI工具链配置、ESP32项目编译、
  固件烧录、串口调试等全流程。适用于ESP32-WROOM-32等常见开发板，解决环境配置、
  库依赖管理、上传失败、串口通信等常见问题。支持Windows/Linux/macOS跨平台开发。
author: EricSun
version: 1.0.0
date: 2026-02-27
---

# ESP32 Arduino 开发完整指南

## 问题
ESP32开发环境配置复杂，涉及工具链安装、板级支持包配置、库依赖管理、烧录端口选择等多个环节，容易出现编译失败、上传超时、串口无法识别等问题。

## 上下文 / 触发条件
- 新建ESP32项目需要搭建开发环境
- 编译时出现" Board not found"或库缺失错误
- 上传固件时提示"Failed to connect to ESP32"
- 串口无法识别或通信失败
- 需要批量编译和烧录多个ESP32设备

## 解决方案

### 1. 工具链安装与配置

#### 1.1 安装 Arduino CLI

**Windows (PowerShell):**
```powershell
# 下载最新版
Invoke-WebRequest -Uri "https://downloads.arduino.cc/arduino-cli/arduino-cli_latest_Windows_64bit.zip" -OutFile "arduino-cli.zip"
Expand-Archive -Path "arduino-cli.zip" -DestinationPath "$env:USERPROFILE\bin"
[Environment]::SetEnvironmentVariable("Path", $env:Path + ";$env:USERPROFILE\bin", "User")
```

**Linux/macOS:**
```bash
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
sudo mv bin/arduino-cli /usr/local/bin/
```

#### 1.2 初始化配置

```bash
# 创建默认配置文件
arduino-cli config init

# 查看当前配置
arduino-cli config dump

# 添加ESP32板级支持包地址
arduino-cli config add board_manager.additional_urls https://dl.espressif.com/dl/package_esp32_index.json
```

#### 1.3 安装ESP32核心

```bash
# 更新板级支持包索引
arduino-cli core update-index

# 安装ESP32核心 (包含所有ESP32系列)
arduino-cli core install esp32:esp32

# 验证安装
arduino-cli core list
```

### 2. 项目结构

#### 2.1 标准Arduino项目结构

```
project-name/
├── project-name.ino          # 主程序文件（必须与目录同名）
├── config.h                  # 配置文件（可选）
├── src/                      # 额外源文件目录
│   ├── utils.cpp
│   └── utils.h
├── lib/                      # 本地库目录（可选）
│   └── MyLibrary/
│       ├── MyLibrary.h
│       └── MyLibrary.cpp
└── sketch.yaml               # 项目配置文件（Arduino CLI 1.0+）
```

#### 2.2 sketch.yaml 项目配置

```yaml
# sketch.yaml - 放在项目根目录
profiles:
  default:
    fqbn: esp32:esp32:esp32    # 完全限定板子名称
    platforms:
      - platform: esp32:esp32 (2.0.14)
    libraries:
      - ArduinoJson (6.21.3)
      - PubSubClient (2.8)
```

### 3. 编译流程

#### 3.1 基本编译命令

```bash
# 进入项目目录
cd project-name/

# 编译（自动检测当前目录的.ino文件）
arduino-cli compile --fqbn esp32:esp32:esp32

# 详细输出（调试用）
arduino-cli compile --fqbn esp32:esp32:esp32 --verbose

# 指定输出目录
arduino-cli compile --fqbn esp32:esp32:esp32 --output-dir ./build
```

#### 3.2 常见FQBN（完全限定板子名称）

| 开发板型号 | FQBN |
|-----------|------|
| ESP32 Dev Module | `esp32:esp32:esp32` |
| ESP32-S2 | `esp32:esp32:esp32s2` |
| ESP32-S3 | `esp32:esp32:esp32s3` |
| ESP32-C3 | `esp32:esp32:esp32c3` |
| ESP32-C6 | `esp32:esp32:esp32c6` |
| NodeMCU-32S | `esp32:esp32:nodemcu-32s` |
| ESP32-CAM | `esp32:esp32:esp32cam` |

#### 3.3 编译选项

```bash
# 指定分区表（大程序需要）
arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=huge_app

# 指定Flash大小
arduino-cli compile --fqbn esp32:esp32:esp32:FlashSize=4MB

# 指定CPU频率
arduino-cli compile --fqbn esp32:esp32:esp32:CPUFreq=240

# 调试级别
arduino-cli compile --fqbn esp32:esp32:esp32:DebugLevel=debug
```

### 4. 烧录固件

#### 4.1 识别串口

```bash
# 列出所有可用串口
arduino-cli board list

# 示例输出：
# Port                            Protocol Type              Board Name   FQBN            Core
# /dev/ttyUSB0 (Linux)            serial   Serial Port (USB) ESP32 Dev... esp32:esp32:... esp32:esp32
# COM11 (Windows)                 serial   Serial Port (USB) ESP32 Dev... esp32:esp32:... esp32:esp32
```

#### 4.2 上传命令

```bash
# 基本上传（指定端口和FQBN）
arduino-cli upload --fqbn esp32:esp32:esp32 -p COM11

# 或 /dev/ttyUSB0 (Linux)
arduino-cli upload --fqbn esp32:esp32:esp32 -p /dev/ttyUSB0

# 上传前自动编译
arduino-cli upload --fqbn esp32:esp32:esp32 -p COM11 --verify
```

#### 4.3 进入下载模式

ESP32上传需要进入下载模式，不同板子操作不同：

**带自动下载电路的板子（如NodeMCU-32S）:**
- 直接上传，无需手动操作

**需要手动进入下载模式的板子:**
1. 按住 **BOOT** 按钮
2. 按一下 **EN** (Reset) 按钮
3. 松开 **BOOT** 按钮
4. 立即执行上传命令

**Windows一键上传脚本:**
```powershell
# upload.ps1
$port = "COM11"
$fqbn = "esp32:esp32:esp32"

Write-Host "Uploading to $port..."
arduino-cli upload --fqbn $fqbn -p $port --verify
if ($LASTEXITCODE -eq 0) {
    Write-Host "Upload successful!" -ForegroundColor Green
} else {
    Write-Host "Upload failed! Try manually entering download mode." -ForegroundColor Red
}
```

### 5. 库管理

#### 5.1 安装库

```bash
# 搜索库
arduino-cli lib search "ArduinoJson"

# 安装库
arduino-cli lib install "ArduinoJson"
arduino-cli lib install "PubSubClient"

# 安装特定版本
arduino-cli lib install "ArduinoJson@6.21.3"

# 从Git URL安装
arduino-cli lib install --git-url https://github.com/user/library.git

# 从ZIP安装
arduino-cli lib install --zip-path ./MyLibrary.zip
```

#### 5.2 常用ESP32库清单

```bash
# 网络通信
arduino-cli lib install "ArduinoJson"           # JSON处理
arduino-cli lib install "PubSubClient"          # MQTT客户端
arduino-cli lib install "WiFiManager"           # WiFi配网

# 传感器
arduino-cli lib install "DHT sensor library"    # DHT温湿度
arduino-cli lib install "Adafruit Unified Sensor"
arduino-cli lib install "BMP280_DEV"            # 气压传感器

# 显示
arduino-cli lib install "TFT_eSPI"              # TFT显示屏
arduino-cli lib install "U8g2"                  # OLED显示屏

# 其他
arduino-cli lib install "ESPAsyncWebServer"     # 异步Web服务器
arduino-cli lib install "AsyncTCP"
```

#### 5.3 本地库开发

将库放在项目目录的 `lib/` 子目录下：

```
MyProject/
├── MyProject.ino
└── lib/
    └── MyCustomLib/
        ├── MyCustomLib.h
        └── MyCustomLib.cpp
```

### 6. 串口调试

#### 6.1 Arduino CLI 串口监视器

```bash
# 打开串口监视器
arduino-cli monitor -p COM11

# 指定波特率
arduino-cli monitor -p COM11 --config baudrate=115200

# 支持的波特率: 300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 74880, 115200, 230400, 250000, 500000, 1000000, 2000000
```

#### 6.2 使用其他串口工具

**Windows - PuTTY:**
- Connection type: Serial
- Serial line: COM11
- Speed: 115200

**跨平台 - minicom/picocom:**
```bash
# Linux/macOS
minicom -D /dev/ttyUSB0 -b 115200
picocom -b 115200 /dev/ttyUSB0
```

**Python串口工具:**
```python
import serial

ser = serial.Serial('COM11', 115200, timeout=1)
while True:
    line = ser.readline().decode('utf-8').strip()
    if line:
        print(line)
```

### 7. 完整开发工作流脚本

#### 7.1 Windows批处理脚本 (build.bat)

```batch
@echo off
setlocal enabledelayedexpansion

set PROJECT_NAME=esp32-wifi-controller
set FQBN=esp32:esp32:esp32
set PORT=COM11

echo ========================================
echo ESP32 Arduino Build Script
echo Project: %PROJECT_NAME%
echo ========================================

:: Find project directory
if exist "%PROJECT_NAME%/%PROJECT_NAME%.ino" (
    cd "%PROJECT_NAME%"
) else if exist "%PROJECT_NAME%.ino" (
    echo Already in project directory
) else (
    echo Error: Cannot find %PROJECT_NAME%.ino
    exit /b 1
)

echo.
echo [1/4] Checking Arduino CLI...
arduino-cli version >nul 2>&1
if errorlevel 1 (
    echo Error: Arduino CLI not found in PATH
    exit /b 1
)

echo.
echo [2/4] Updating core index...
arduino-cli core update-index

echo.
echo [3/4] Compiling...
arduino-cli compile --fqbn %FQBN% --verbose
if errorlevel 1 (
    echo.
    echo Compilation FAILED!
    exit /b 1
)

echo.
echo [4/4] Uploading to %PORT%...
echo If upload fails, press and hold BOOT, then press EN, release BOOT, and retry.
arduino-cli upload --fqbn %FQBN% -p %PORT%
if errorlevel 1 (
    echo.
    echo Upload FAILED!
    exit /b 1
)

echo.
echo ========================================
echo Build and Upload SUCCESSFUL!
echo ========================================
timeout /t 3
```

#### 7.2 Linux/macOS Shell脚本 (build.sh)

```bash
#!/bin/bash

PROJECT_NAME="esp32-wifi-controller"
FQBN="esp32:esp32:esp32"
PORT="/dev/ttyUSB0"  # 根据实际修改

echo "========================================"
echo "ESP32 Arduino Build Script"
echo "Project: $PROJECT_NAME"
echo "========================================"

# 检测项目目录
if [ -d "$PROJECT_NAME" ]; then
    cd "$PROJECT_NAME"
fi

# 检查Arduino CLI
if ! command -v arduino-cli &> /dev/null; then
    echo "Error: Arduino CLI not found"
    exit 1
fi

# 自动检测串口
if [ -z "$PORT" ]; then
    PORT=$(arduino-cli board list | grep "esp32" | head -1 | awk '{print $1}')
    if [ -z "$PORT" ]; then
        echo "Error: No ESP32 device found"
        exit 1
    fi
    echo "Auto-detected port: $PORT"
fi

echo ""
echo "[1/3] Compiling..."
arduino-cli compile --fqbn "$FQBN" || exit 1

echo ""
echo "[2/3] Uploading to $PORT..."
echo "If fails: Hold BOOT, press EN, release BOOT"
sudo arduino-cli upload --fqbn "$FQBN" -p "$PORT" || exit 1

echo ""
echo "[3/3] Opening serial monitor..."
sleep 1
arduino-cli monitor -p "$PORT" -c baudrate=115200
```

### 8. 常见问题与解决

#### 8.1 "Failed to connect to ESP32: Timed out waiting for packet header"

**原因：** ESP32未进入下载模式

**解决：**
1. 按住 **BOOT** 按钮
2. 按一下 **EN** 按钮
3. 松开 **BOOT** 按钮
4. 立即执行上传命令

#### 8.2 "A fatal error occurred: Failed to connect to ESP32: Invalid head of packet"

**原因：** 串口被占用或波特率不匹配

**解决：**
```bash
# 关闭所有串口监视器
# 检查占用（Linux）
sudo lsof /dev/ttyUSB0

# 降低上传波特率
arduino-cli upload --fqbn esp32:esp32:esp32 -p COM11 -t UploadSpeed=115200
```

#### 8.3 "ESP32: esp32: No board selected"

**原因：** 未安装ESP32核心或FQBN错误

**解决：**
```bash
# 安装ESP32核心
arduino-cli core install esp32:esp32

# 查看可用板子
arduino-cli board listall esp32
```

#### 8.4 "Library not found"

**原因：** 缺少依赖库

**解决：**
```bash
# 安装缺失的库
arduino-cli lib install "LibraryName"

# 或创建libraries目录链接
ln -s /path/to/your/libs ~/Arduino/libraries
```

#### 8.5 串口权限问题 (Linux)

```bash
# 临时解决
sudo chmod 666 /dev/ttyUSB0

# 永久解决 - 将用户加入dialout组
sudo usermod -a -G dialout $USER
# 重新登录生效
```

#### 8.6 编译内存不足

```bash
# 使用更大的分区表
arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=huge_app

# 或最小SPIFFS
arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=min_spiffs
```

### 9. ESP32 Arduino API速查

#### 9.1 GPIO控制

```cpp
// 基础GPIO
pinMode(2, OUTPUT);
digitalWrite(2, HIGH);
int val = digitalRead(4);

// PWM (ESP32 Arduino 3.x API)
ledcAttach(2, 5000, 8);  // pin, freq, resolution_bits
ledcWrite(2, 128);       // duty cycle (0-255 for 8-bit)

// 触摸传感器
touchAttachPin(4, getTouchValue);
int touchVal = touchRead(4);

// DAC (GPIO25, GPIO26)
dacWrite(25, 128);  // 0-255
```

#### 9.2 WiFi连接

```cpp
#include <WiFi.h>

const char* ssid = "YourSSID";
const char* password = "YourPassword";

void setup() {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println(WiFi.localIP());
}
```

#### 9.3 NVS存储 (Preferences)

```cpp
#include <Preferences.h>

Preferences prefs;

void setup() {
    prefs.begin("myapp", false);  // namespace, read-only=false

    // 写入
    prefs.putString("ssid", "MyWiFi");
    prefs.putInt("count", 42);

    // 读取
    String ssid = prefs.getString("ssid", "default");
    int count = prefs.getInt("count", 0);

    prefs.end();
}
```

### 10. 项目模板

#### 10.1 基础项目模板

```cpp
/**
 * @file project.ino
 * @brief ESP32 Project Template
 * @date 2026-02-27
 */

#include <Arduino.h>

// ============ 配置区 ============
#define LED_PIN     2
#define SERIAL_BAUD 115200

// ============ 全局变量 ============
unsigned long lastBlink = 0;
bool ledState = false;

// ============ 函数声明 ============
void setupGPIO();
void setupSerial();
void handleBlink();

// ============ 初始化 ============
void setup() {
    setupSerial();
    setupGPIO();

    Serial.println("ESP32 Started!");
    Serial.print("CPU Frequency: ");
    Serial.print(getCpuFrequencyMhz());
    Serial.println(" MHz");
}

// ============ 主循环 ============
void loop() {
    handleBlink();
    // 添加其他任务
}

// ============ 函数实现 ============
void setupSerial() {
    Serial.begin(SERIAL_BAUD);
    while (!Serial) { ; }  // 等待串口连接 ( Leonardo/Micro等需要)
    delay(100);
}

void setupGPIO() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
}

void handleBlink() {
    unsigned long now = millis();
    if (now - lastBlink >= 1000) {
        lastBlink = now;
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);
        Serial.println(ledState ? "LED ON" : "LED OFF");
    }
}
```

## 验证

成功标志：
1. `arduino-cli core list` 显示 `esp32:esp32` 已安装
2. `arduino-cli board list` 能识别连接的ESP32设备
3. 编译命令成功生成 `.bin` 文件
4. 上传后ESP32正常运行，串口有输出

## 参考资料

- [Arduino CLI 官方文档](https://arduino.github.io/arduino-cli/)
- [ESP32 Arduino 核心文档](https://docs.espressif.com/projects/arduino-esp32/)
- [ESP32 引脚定义参考](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html)
- [ArduinoJson 库文档](https://arduinojson.org/)
