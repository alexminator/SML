# ESP32 Arduino Development

A Claude Code skill for ESP32 development using Arduino CLI toolchain.

## Description

This skill provides expert guidance for ESP32 Arduino development without relying on Arduino IDE. It covers the complete workflow from toolchain installation to firmware flashing and serial debugging via command line.

**Supported Boards:** ESP32-WROOM-32, ESP32-S2, ESP32-S3, ESP32-C3, ESP32-C6, NodeMCU-32S, ESP32-CAM, and compatible

## When to Use This Skill

- Setting up Arduino CLI development environment for ESP32
- Creating build scripts for ESP32 projects
- Configuring board support packages and library dependencies
- Troubleshooting compilation errors and upload failures
- Flashing firmware via serial port using command line
- Understanding ESP32 project structure and Arduino API
- Solving "Failed to connect to ESP32" upload issues
- Serial communication and debugging

## How to Invoke

Mention ESP32 Arduino development in your request:

- "Help me set up an ESP32 project"
- "How do I build ESP32 firmware without Arduino IDE?"
- "esp32 arduino build script"
- "esp32 flash programming"
- "arduino cli configuration"
- "esp32 library management"

## Capabilities

### 1. Toolchain Setup
- Arduino CLI installation guidance
- Environment variable configuration
- ESP32 core installation
- Cross-platform support (Windows/Linux/macOS)

### 2. Project Structure
- Standard Arduino project layout
- sketch.yaml configuration
- Library organization
- Multi-file project setup

### 3. Build Configuration
- FQBN (Fully Qualified Board Name) selection
- Partition scheme configuration
- Flash size and CPU frequency options
- Debug level settings

### 4. Build Automation
- Shell script templates (Windows/Linux)
- Batch file examples
- CI/CD integration

### 5. Flash Programming
- Serial port identification
- Download mode entry procedures
- Upload command examples
- Troubleshooting connection issues

### 6. Library Management
- Installing libraries from registry
- Installing from Git/Zip sources
- Local library development
- Common ESP32 library recommendations

### 7. Serial Debugging
- Arduino CLI monitor usage
- Alternative serial tools (PuTTY, minicom, picocom)
- Python serial scripts

### 8. Troubleshooting
Common errors and solutions:
- "Failed to connect to ESP32" timeout errors
- Invalid packet header errors
- Missing board support package
- Library not found errors
- Serial port permission issues (Linux)
- Compilation memory overflow

### 9. API Quick Reference
- GPIO control and PWM
- WiFi connection
- Touch sensor reading
- DAC output
- NVS (Preferences) storage

## Example Workflow

```
User: "Help me create an ESP32 build script"

Skill will provide:
1. Complete build.bat / build.sh template
2. Arduino CLI configuration guidance
3. FQBN and compilation options
4. Upload command examples
5. Serial monitor setup
```

## Requirements

- Arduino CLI installed
- USB-to-Serial driver (CH340/CP2102/etc.)
- Target ESP32 development board
- USB cable (data capable)

## Output Format

Responses include:
- Ready-to-use shell/batch scripts
- Code snippets with explanations
- Step-by-step procedures
- Troubleshooting tables
- API reference examples

## Author

**EricSun**

## Version

1.0.0

## License

MIT
