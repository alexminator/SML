# ESP32 Arduino 开发指南

用于 ESP32 Arduino 开发的 Claude Code Skill，基于 Arduino CLI 工具链。

## 简介

本 Skill 提供 ESP32 Arduino 开发的专业指导，无需依赖 Arduino IDE。涵盖从工具链安装到命令行固件烧录和串口调试的完整流程。

**支持开发板：** ESP32-WROOM-32、ESP32-S2、ESP32-S3、ESP32-C3、ESP32-C6、NodeMCU-32S、ESP32-CAM 及兼容型号

## 使用场景

- 搭建 ESP32 Arduino CLI 开发环境
- 为 ESP32 项目创建构建脚本
- 配置板级支持包和库依赖
- 排查编译错误和上传失败
- 使用命令行通过串口烧录固件
- 了解 ESP32 项目结构和 Arduino API
- 解决 "Failed to connect to ESP32" 上传问题
- 串口通信与调试

## 调用方式

在请求中提及 ESP32 Arduino 开发：

- "帮我搭建一个 ESP32 项目"
- "如何不用 Arduino IDE 构建 ESP32 固件？"
- "esp32 arduino 构建脚本"
- "esp32 烧录编程"
- "arduino cli 配置"
- "esp32 库管理"

## 能力范围

### 1. 工具链配置
- Arduino CLI 安装指导
- 环境变量配置
- ESP32 核心安装
- 跨平台支持（Windows/Linux/macOS）

### 2. 项目结构
- 标准 Arduino 项目布局
- sketch.yaml 配置文件
- 库组织方式
- 多文件项目设置

### 3. 构建配置
- FQBN（完全限定板子名称）选择
- 分区表配置
- Flash 大小和 CPU 频率选项
- 调试级别设置

### 4. 构建自动化
- Shell 脚本模板（Windows/Linux）
- 批处理文件示例
- CI/CD 集成方案

### 5. 烧录编程
- 串口识别方法
- 下载模式进入步骤
- 上传命令示例
- 连接问题排查

### 6. 库管理
- 从注册表安装库
- 从 Git/Zip 源安装
- 本地库开发
- 常用 ESP32 库推荐

### 7. 串口调试
- Arduino CLI 监视器用法
- 替代串口工具（PuTTY、minicom、picocom）
- Python 串口脚本

### 8. 故障排查
常见错误及解决方案：
- "Failed to connect to ESP32" 超时错误
- 无效包头错误
- 缺少板级支持包
- 库未找到错误
- 串口权限问题（Linux）
- 编译内存溢出

### 9. API 速查
- GPIO 控制和 PWM
- WiFi 连接
- 触摸传感器读取
- DAC 输出
- NVS（Preferences）存储

## 示例工作流

```
用户："帮我创建一个 ESP32 构建脚本"

Skill 将提供：
1. 完整的 build.bat / build.sh 模板
2. Arduino CLI 配置指导
3. FQBN 和编译选项
4. 上传命令示例
5. 串口监视器设置
```

## 环境要求

- 已安装 Arduino CLI
- USB转串口驱动（CH340/CP2102等）
- 目标 ESP32 开发板
- USB 数据线（支持数据传输）

## 输出格式

回复内容包括：
- 可直接使用的 shell/批处理脚本
- 带解释的代码片段
- 分步操作流程
- 故障排查对照表
- API 参考示例

## 作者

**EricSun**

## 版本

1.0.0

## 许可证

MIT
