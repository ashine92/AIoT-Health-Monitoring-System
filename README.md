# AIoT-Health-Monitoring-System

## Overview
This project implements a wearable AIoT (Artificial Intelligence + Internet of Things) health monitoring system using ESP32-S3. The system continuously collects vital signs from multiple sensors, performs AI-based risk analysis, and displays results on a TFT screen. The solution targets real-time and predictive healthcare monitoring, particularly for elderly people and patients with chronic diseases.

## Hardware Components
- ESP32-S3 Super Mini (WiFi + BLE): Central microcontroller with dual-core processing.
- MAX30102: Heart rate and SpO₂ sensor using PPG (Photoplethysmography).
- MLX90614: Contactless body temperature sensor with ±0.5°C accuracy.
- MPU6050: 6DOF IMU (accelerometer + gyroscope) to track movement and posture.
- DS1302 RTC Module: Real-time clock for time stamping data.
- ST7789 TFT Display: Displays sensor data and AI inference results.
- Power: TP4056 charging module and LiPo battery (1300 mAh).

## Software Components
- Language: C++ (ESP-IDF / Arduino framework)
- AI Model: DNN (Deep Neural Network) quantized with ESP-DL, stored as .espdl binary in flash.
- Data Preprocessing: Standardization using feature-wise mean and standard deviation.
- Inference Pipeline:
+ Collect sensor data (Heart Rate, SpO₂, Body Temp, Steps, Fall detection, Activity, Acceleration).
+ Normalize data.
+ Run the DNN model.
+ Apply sigmoid to output logits to classify risk as Low Risk or High Risk.

## Features
1. Sensor Data Collection
- HR and SpO₂ via MAX30102
- Body temperature via MLX90614
- Step counting via MPU6050
- Simulated demographic data: Age, Gender, Weight, Height
3. AI Inference
- Runs on-device AI model (.espdl) to classify health risk.
- Real-time risk assessment displayed on TFT.
4. Display
- Shows vital signs, step count, and risk classification.
- Color-coded output: green for Low Risk, red for High Risk.
5. RTOS Tasks
- Separate task prints RTC time every second.
- Main loop collects sensor data and performs inference.

## AI Model Details
- Input features: HR, SpO2, Body Temperature, Age, Weight, Height, Gender
- Output: Health risk (Low/High)
- Normalization: `(value - mean) / std`
- Post-processing: Sigmoi activation -> probability -> classification.

## Mobile Application
- Developed in Flutter, with Firebase backend for data storage
- Displays user historical data and real-time readings



