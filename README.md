# AIoT Health Monitoring System

A wearable AIoT (Artificial Intelligence + Internet of Things) health monitoring system powered by ESP32-S3 that continuously monitors vital signs and provides real-time AI-powered health risk assessment.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32--S3-green.svg)

## 🎯 Overview

This project implements an intelligent wearable health monitoring device that collects vital signs from multiple sensors, processes data using on-device AI inference, and provides real-time health risk classification. The system combines edge computing with mobile connectivity to deliver comprehensive health monitoring capabilities.

## 🔧 Hardware Components

| Component | Description | Purpose |
|-----------|-------------|---------|
| **ESP32-S3 Super Mini** | Dual-core microcontroller with WiFi + BLE | Central processing unit |
| **MAX30102** | PPG sensor | Heart rate and SpO₂ monitoring |
| **MLX90614** | Contactless IR sensor (±0.5°C accuracy) | Body temperature measurement |
| **MPU6050** | 6-axis IMU (accelerometer + gyroscope) | Movement tracking, step counting, fall detection |
| **DS1302** | Real-time clock module | Data timestamping |
| **ST7789** | TFT Display | Visual feedback and data display |
| **TP4056** | Charging module + 1300mAh LiPo battery | Power management |

## 💻 Software Architecture

### Technology Stack
- **Framework**: ESP-IDF / Arduino
- **Language**: C++ (embedded), Python (model training), Flutter (mobile app)
- **AI Engine**: ESP-DL (Espressif Deep Learning Library)
- **Backend**: Firebase for data storage and synchronization

### AI Model
- **Type**: Deep Neural Network (DNN)
- **Format**: Quantized .espdl binary stored in flash
- **Input Features**: 
  - Heart Rate (HR)
  - Blood Oxygen Saturation (SpO₂)
  - Body Temperature
  - Age
  - Weight
  - Height
  - Gender
- **Output**: Binary health risk classification (Low Risk / High Risk)
- **Preprocessing**: Feature-wise standardization using mean and standard deviation
- **Post-processing**: Sigmoid activation for probability estimation

## ✨ Key Features

### 1. Multi-Sensor Data Collection
- **Cardiovascular Monitoring**: Real-time heart rate and SpO₂ via MAX30102
- **Temperature Sensing**: Non-contact body temperature measurement via MLX90614
- **Activity Tracking**: Step counting and movement analysis via MPU6050
- **Fall Detection**: Automatic fall detection using accelerometer data
- **Demographic Data**: Age, gender, weight, and height parameters

### 2. On-Device AI Inference
- Edge computing for real-time health risk assessment
- Low-latency predictions without cloud dependency
- Privacy-preserving local processing
- Optimized DNN model for ESP32-S3

### 3. Visual Display
- Real-time vital signs visualization on ST7789 TFT
- Color-coded risk indicators (🟢 Green: Low Risk, 🔴 Red: High Risk)
- Step counter and activity status
- Timestamp display via RTC module

### 4. RTOS Task Management
- **Time Display Task**: Updates RTC time every second
- **Sensor Collection Task**: Continuous data acquisition from all sensors
- **Inference Task**: Periodic AI model execution
- **Display Task**: UI updates and rendering

### 5. Mobile Application
- Built with Flutter for cross-platform compatibility
- Firebase integration for cloud storage
- Historical data visualization
- Real-time synchronization with wearable device

## 📊 Data Processing Pipeline

```
┌─────────────────┐
│  Sensor Data    │
│  Collection     │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Normalization   │
│ (value-mean)/std│
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  DNN Inference  │
│   (ESP-DL)      │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│Sigmoid Activation│
│  & Classification│
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Display Results │
│  & Send to App  │
└─────────────────┘
```

## 🚀 Getting Started

### Prerequisites
- ESP-IDF v4.4 or later
- Python 3.7+ (for model development)
- Flutter SDK (for mobile app)
- Firebase account

### Hardware Setup
1. Connect sensors to ESP32-S3 according to pinout configuration
2. Install battery and charging module
3. Upload firmware to ESP32-S3

### Software Installation

```bash
# Clone the repository
git clone https://github.com/ashine92/AIoT-Health-Monitoring-System.git
cd AIoT-Health-Monitoring-System

# Initialize ESP-IDF environment
. $HOME/esp/esp-idf/export.sh

# Build and flash the firmware
cd model_inference/human_vital_signs
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## 📁 Project Structure

```
AIoT-Health-Monitoring-System/
├── model_development/          # AI model training notebooks
│   └── *.ipynb                 # Jupyter notebooks for model development
├── model_inference/            # ESP32 firmware
│   └── human_vital_signs/      # Main inference application
│       ├── main/               # Source code
│       └── model/              # Trained .espdl model files
├── mobile_app/                 # Flutter application (if applicable)
└── README.md                   # This file
```

## 🔬 Model Development

The AI model is developed using Jupyter Notebooks in the `model_development/` directory. The training pipeline includes:

1. Data collection and preprocessing
2. Feature engineering and normalization
3. DNN model architecture design
4. Training and validation
5. Model quantization for ESP-DL
6. Conversion to .espdl format

## 📱 Mobile Application Features

- Real-time vital signs dashboard
- Historical data charts and trends
- Health risk notifications
- User profile management
- Cloud data synchronization via Firebase

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.

## 👥 Authors

- **ashine92** - Initial work

## 🙏 Acknowledgments

- Espressif Systems for ESP-IDF and ESP-DL
- Open-source sensor library contributors
- Firebase for backend infrastructure

## 📧 Contact

For questions or support, please open an issue on GitHub.

---

**Note**: This is an educational/research project. For medical-grade health monitoring, please consult certified medical devices and healthcare professionals.
