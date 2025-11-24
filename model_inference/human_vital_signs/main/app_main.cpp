// ============== FULL Phần AI =================
/*
#include "dl_model_base.hpp"
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <cmath>

#define NUM_SAMPLES 1
#define NUM_FEATURES 7
// Tên biến chính xác sinh bởi linker từ "models/espdl_model.espdl"
extern const uint8_t model_espdl[] asm("_binary_model_espdl_start");

// Hàm sigmoid 
float sigmoid(float x) {
    return 1.0f / (1.0f + expf(-x));
}

// Dữ liệu thô
float input_data[NUM_SAMPLES][NUM_FEATURES] = {
    //{96, 36.85263343f, 97.12412468f, 72, 0, 71.75897167f, 1.603887873f} // High Risk
    {83, 36.044191f,   98.584497f,   84, 0, 79.295332f,   1.672735f} // Low Risk
    // {63, 37.052, 98.50, 68, 1, 90.31, 1.77}
    //{79, 36.885, 95.9871, 22, 0, 79.8699, 1.92233}
};

const float means[NUM_FEATURES] = {79.53374663, 36.74835291, 97.50437243, 53.44627537, 0.49946505, 74.99641903, 1.75003102};
const float stds[NUM_FEATURES] = {11.55286498, 0.43328918, 1.44259433, 20.78674961, 0.49999971, 14.4714659, 0.14455348};

float standard_scaler(float x, float u, float s) {
    float z = (x - u) / s;
    return z;
}

void run_hvsd_model_batch()
{
    // Load model từ flash
    dl::Model *model = new dl::Model((const char *)model_espdl, fbs::MODEL_LOCATION_IN_FLASH_RODATA);

    ESP_ERROR_CHECK(model->test());
    // Lấy input/output tensor
    dl::TensorBase *model_input = model->get_inputs().begin()->second;
    dl::TensorBase *model_output = model->get_outputs().begin()->second;

    // Chuẩn bị dữ liệu input chuẩn hóa
    float input_scaled[NUM_SAMPLES][NUM_FEATURES];
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        printf("Sample %d:\n", i);
        for (int j = 0; j < NUM_FEATURES; ++j) {
            input_scaled[i][j] = (input_data[i][j] - means[j]) / stds[j];
            printf("Feature %d: raw=%.4f, mean=%.4f, std=%.4f, normalized=%.4f\n", j, input_data[i][j], means[j], stds[j], input_scaled[i][j]);
        }
    }

    // Gán dữ liệu cho input tensor
    dl::TensorBase* input_tensor = new dl::TensorBase({NUM_SAMPLES, NUM_FEATURES}, nullptr, 0, dl::DATA_TYPE_FLOAT);
    memcpy(input_tensor->data, input_scaled, sizeof(input_scaled));
    model_input->assign(input_tensor);

    // Chạy mô hình
    model->run();

    // Lấy và đọc output
    printf("Model output shape: [%zu, %zu]\n", model_output->shape[0], model_output->shape[1]);
    float* raw_output_data = (float*)model_output->data;
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        printf("Raw model output[%d] = %f\n", i, raw_output_data[i]);
    }
    dl::TensorBase* output_tensor = new dl::TensorBase({NUM_SAMPLES, 1}, nullptr, 0, dl::DATA_TYPE_FLOAT);

    output_tensor->assign(model_output);

    float *output_ptr = (float *)output_tensor->data;
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        float logit = output_ptr[i];
        float prob = sigmoid(logit);
        int class_label = (prob >= 0.5f) ? 1 : 0;
        const char* risk = (class_label == 1) ? "Low Risk" : "High Risk";

        printf("Sample %d: logit = %.4f, prob = %.4f -> %s\n", i, logit, prob, risk);

    }

    delete input_tensor;
    delete output_tensor;
    delete model;

    printf("Inference done.\n");
}

// ===== Load & Test & Profile Model =====
extern "C" void app_main(void)
{
    run_hvsd_model_batch();
}
*/// ============== END FULL Phần AI =================

/* Phần AI giả lập dữ liệu (14/10) - Part 1 - Giả lập toàn bộ input*/
/*
// ============== FULL Phần AI =================

#include "dl_model_base.hpp"
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <cmath>
#include <cstdlib>     // rand(), srand()
#include <ctime>       // time()
#include <unistd.h>    // usleep() hoặc vTaskDelay cho ESP-IDF

#define NUM_SAMPLES 1
#define NUM_FEATURES 7

// Tên biến chính xác sinh bởi linker từ "models/espdl_model.espdl"
extern const uint8_t model_espdl[] asm("_binary_model_espdl_start");

// ====================== HÀM TIỆN ÍCH =========================

// Sigmoid
float sigmoid(float x) {
    return 1.0f / (1.0f + expf(-x));
}

// Chuẩn hóa dữ liệu
float standard_scaler(float x, float u, float s) {
    return (x - u) / s;
}

// Trung bình và độ lệch chuẩn
const float means[NUM_FEATURES] = {79.53374663, 36.74835291, 97.50437243, 53.44627537, 0.49946505, 74.99641903, 1.75003102};
const float stds[NUM_FEATURES] = {11.55286498, 0.43328918, 1.44259433, 20.78674961, 0.49999971, 14.4714659, 0.14455348};

// ====================== GIẢ LẬP DỮ LIỆU =========================

// Giả lập dữ liệu cảm biến: HR, Temp, SpO2, Step, Fall, Activity, Acceleration
void simulate_input_data(float input_data[NUM_SAMPLES][NUM_FEATURES]) {
    for (int i = 0; i < NUM_SAMPLES; i++) {
        // Tạo biến động nhẹ quanh giá trị trung bình sinh lý bình thường
        float heart_rate = 60 + rand() % 60;                  // 60–120 bpm
        float temperature = 36.0f + ((rand() % 150) / 100.0f); // 36.0–37.5 °C
        float spo2 = 95.0f + ((rand() % 50) / 10.0f);          // 95.0–100.0%
        float steps = 30 + rand() % 100;                       // 30–130 steps/min
        float fall_flag = (rand() % 20 == 0) ? 1.0f : 0.0f;    // Xác suất nhỏ xảy ra ngã
        float activity = 60 + rand() % 40;                     // 60–100 (mức độ vận động)
        float accel = 1.5f + ((rand() % 50) / 100.0f);         // 1.5–2.0 g

        input_data[i][0] = heart_rate;
        input_data[i][1] = temperature;
        input_data[i][2] = spo2;
        input_data[i][3] = steps;
        input_data[i][4] = fall_flag;
        input_data[i][5] = activity;
        input_data[i][6] = accel;

        printf("\n[SIM] Input sample generated:\n");
        printf("HR=%.2f, Temp=%.2f°C, SpO2=%.2f%%, Steps=%.2f, Fall=%.0f, Activity=%.2f, Acc=%.2f\n",
               heart_rate, temperature, spo2, steps, fall_flag, activity, accel);
    }
}

// ====================== CHẠY MÔ HÌNH =========================

void run_hvsd_model_once(float input_data[NUM_SAMPLES][NUM_FEATURES])
{
    dl::Model *model = new dl::Model((const char *)model_espdl, fbs::MODEL_LOCATION_IN_FLASH_RODATA);
    ESP_ERROR_CHECK(model->test());

    dl::TensorBase *model_input = model->get_inputs().begin()->second;
    dl::TensorBase *model_output = model->get_outputs().begin()->second;

    // Chuẩn hóa dữ liệu
    float input_scaled[NUM_SAMPLES][NUM_FEATURES];
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        for (int j = 0; j < NUM_FEATURES; ++j) {
            input_scaled[i][j] = standard_scaler(input_data[i][j], means[j], stds[j]);
        }
    }

    // Gán dữ liệu cho tensor
    dl::TensorBase* input_tensor = new dl::TensorBase({NUM_SAMPLES, NUM_FEATURES}, nullptr, 0, dl::DATA_TYPE_FLOAT);
    memcpy(input_tensor->data, input_scaled, sizeof(input_scaled));
    model_input->assign(input_tensor);

    // Chạy mô hình
    model->run();

    float* raw_output_data = (float*)model_output->data;

    // Giải thích kết quả
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        float logit = raw_output_data[i];
        float prob = sigmoid(logit);
        int class_label = (prob >= 0.5f) ? 1 : 0;
        const char* risk = (class_label == 1) ? "Low Risk" : "High Risk";
        printf("→ Output[%d]: logit=%.4f, prob=%.4f → %s\n", i, logit, prob, risk);
    }

    delete input_tensor;
    delete model;
}

// ====================== VÒNG LẶP LIÊN TỤC =========================

extern "C" void app_main(void)
{
    srand(time(NULL)); // Khởi tạo random seed

    while (true) {
        float input_data[NUM_SAMPLES][NUM_FEATURES];
        simulate_input_data(input_data);      // Sinh dữ liệu ngẫu nhiên
        run_hvsd_model_once(input_data);      // Chạy mô hình
        printf("--------------------------------------------------\n");
        sleep(3); // mỗi 3 giây cập nhật dữ liệu một lần
    }
}
*/

/* Phần AI giả lập dữ liệu (14/10) - Part 2 - Lấy dữ liệu từ cảm biến (MAX30102, MLX90614), giả lập Age, Gender, Height, Weight */
/*
#include "dl_model_base.hpp"
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <unistd.h>

// Cảm biến
#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include "Adafruit_MLX90614.h"
#include "Adafruit_Sensor.h"
#include "Adafruit_MPU6050.h"

#define NUM_SAMPLES 1
#define NUM_FEATURES 7

extern const uint8_t model_espdl[] asm("_binary_model_espdl_start");

// Đối tượng cảm biến
MAX30105 particleSensor;
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
Adafruit_MPU6050 mpu;

// ====== Biến đếm bước chân ======
int stepCount = 0;
float lastAccelMag = 0;
float lastMagnitude = 0;
bool stepFlag = false;
unsigned long lastStepTime = 0;

// ======================== Sigmoid ========================
float sigmoid(float x) {
    return 1.0f / (1.0f + expf(-x));
}

// ======================== Hàm update bước chân từ MPU6050 ========================
void update_step_count() {
    sensors_event_t accelEvent, gyroEvent, tempEvent;
    mpu.getEvent(&accelEvent, &gyroEvent, &tempEvent);

    float magnitude = sqrt(
        accelEvent.acceleration.x*accelEvent.acceleration.x +
        accelEvent.acceleration.y*accelEvent.acceleration.y +
        accelEvent.acceleration.z*accelEvent.acceleration.z
    );

    unsigned long now = millis();
    if (!stepFlag && abs(magnitude - lastMagnitude) > 3.0 && (now - lastStepTime) > 400) {
        stepCount++;
        stepFlag = true;
        lastStepTime = now;
    } else if (stepFlag && abs(magnitude - lastMagnitude) < 1.5) {
        stepFlag = false;
    }
    lastMagnitude = magnitude;
}


// ======================== Chuẩn hóa =======================
const float means[NUM_FEATURES] = {79.53374663, 36.74835291, 97.50437243, 53.44627537, 0.49946505, 74.99641903, 1.75003102};
const float stds[NUM_FEATURES]  = {11.55286498, 0.43328918, 1.44259433, 20.78674961, 0.49999971, 14.4714659,  0.14455348};

// ======================== Hàm đọc cảm biến =======================
bool get_sensor_input_data(float input_data[NUM_SAMPLES][NUM_FEATURES]) {
    static uint32_t irBuffer[100];  // lưu giá trị IR từ MAX30102
    static uint32_t redBuffer[100]; // lưu giá trị RED từ MAX30102
    int32_t bufferLength = 100;

    // Đọc dữ liệu MAX30102
    for (int i = 0; i < bufferLength; i++) {
        while (!particleSensor.check()) {
            // chờ có mẫu mới
        }
        redBuffer[i] = particleSensor.getRed();
        irBuffer[i]  = particleSensor.getIR();
    }

    int32_t spo2;
    int8_t validSPO2;
    int32_t heartRate;
    int8_t validHeartRate;

    // Thuật toán tính HR và SpO2
    maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer,
                                           &spo2, &validSPO2, &heartRate, &validHeartRate);

    // Đọc nhiệt độ từ MLX90614
    float bodyTemp = mlx.readObjectTempC();

    if ((!validSPO2 || !validHeartRate) || (spo2 <= 94 || spo2 >= 100) || (heartRate <= 50 || heartRate >= 130)) {
        printf(" - Heart Rate: %d bpm\n", (int)heartRate);
        printf(" - SpO2: %d%%\n", (int)spo2);
        return false;
    }

    // Gán dữ liệu cho input model
    input_data[0][0] = heartRate;     // Heart Rate
    input_data[0][1] = bodyTemp;      // Body Temperature (°C)
    input_data[0][2] = spo2;          // SpO2
    input_data[0][3] = 22;            // Age
    input_data[0][4] = 0;             // Gender: 0 = Nữ
    input_data[0][5] = 70;            // Weight (kg)
    input_data[0][6] = 1.70;          // Height (m)

    printf("\n[Dữ liệu cảm biến]\nHR = %.1f bpm | SpO2 = %.1f%% | Temp = %.2f °C\n",
           input_data[0][0], input_data[0][2], input_data[0][1]);
    return true;
}

// ======================== Hàm chạy mô hình =======================
void run_hvsd_model(float input_data[NUM_SAMPLES][NUM_FEATURES])
{
    dl::Model *model = new dl::Model((const char *)model_espdl, fbs::MODEL_LOCATION_IN_FLASH_RODATA);
    ESP_ERROR_CHECK(model->test());

    dl::TensorBase *model_input = model->get_inputs().begin()->second;
    dl::TensorBase *model_output = model->get_outputs().begin()->second;

    // Chuẩn hóa dữ liệu
    float input_scaled[NUM_SAMPLES][NUM_FEATURES];
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        for (int j = 0; j < NUM_FEATURES; ++j) {
            input_scaled[i][j] = (input_data[i][j] - means[j]) / stds[j];
        }
    }

    dl::TensorBase* input_tensor = new dl::TensorBase({NUM_SAMPLES, NUM_FEATURES}, nullptr, 0, dl::DATA_TYPE_FLOAT);
    memcpy(input_tensor->data, input_scaled, sizeof(input_scaled));
    model_input->assign(input_tensor);

    // Chạy mô hình
    model->run();

    // Lấy output
    float* raw_output_data = (float*)model_output->data;
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        float logit = raw_output_data[i];
        float prob = sigmoid(logit);
        int class_label = (prob >= 0.5f) ? 1 : 0;
        const char* risk = (class_label == 1) ? "Low Risk" : "High Risk";

        printf("\n===== Kết quả suy luận AI =====\n");
        printf("HR=%.1f | Temp=%.2f | SpO2=%.2f | Age=%.0f | Gender=%s | W=%.1f | H=%.2f\n",
               input_data[i][0], input_data[i][1], input_data[i][2], input_data[i][3],
               (input_data[i][4] == 0 ? "Female" : "Male"),
               input_data[i][5], input_data[i][6]);
        printf("=> Logit=%.4f | Prob=%.4f -> %s\n", logit, prob, risk);
    }

    delete input_tensor;
    delete model;
}

// ======================== Setup cảm biến =======================
void setup_sensors() {
    Wire.begin(9, 8, 400000);
    if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
        printf("❌ Không tìm thấy MAX30102!\n");
        while (1);
    }
    particleSensor.setup(80, 4, 2, 100, 411, 4096); // cấu hình mặc định
    // particleSensor.setPulseAmplitudeRed(0x1F);
    // particleSensor.setPulseAmplitudeIR(0x1F);

    if (!mlx.begin()) {
        printf("❌ Không tìm thấy MLX90614!\n");
        while (1);
    }

    if (!mpu.begin()) {
        printf("❌ Không tìm thấy MPU6050!\n");
        while (1);
    }

    printf("✅ Cảm biến đã sẵn sàng.\n");
}

// ======================== app_main =======================
extern "C" void app_main(void)
{
    setup_sensors();

    while (true) {
        update_step_count();

        float input_data[NUM_SAMPLES][NUM_FEATURES];
        if (get_sensor_input_data(input_data)) {
            run_hvsd_model(input_data);
        } else {
            printf("⚠️  Không có dữ liệu hợp lệ, thử lại...\n");
        }
        
        printf("Tổng bước chân: %d bước\n", stepCount);
        sleep(0.5);  // đọc lại sau 0.5 giây
        printf("-------------------------------------------\n");
    }
}
*/

/* Phần AI lấy dữ liệu thật (19/10) - Part 3 - Lấy dữ liệu từ cảm biến (MAX30102, MLX90614, MPU6050), giả lập Age, Gender, Height, Weight + RTC Module*/
/*
#include "dl_model_base.hpp"
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <unistd.h>

#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include "Adafruit_MLX90614.h"

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include "DS1302.h"  // Thư viện DS1302 bạn cài phải phù hợp

// FreeRTOS delay helpers (ESP)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ====== cấu hình ======
#define NUM_SAMPLES 1
#define NUM_FEATURES 7

// DS1302 pin theo bạn cung cấp
#define DS1302_RST   2
#define DS1302_IO    3
#define DS1302_CLK   4

extern const uint8_t model_espdl[] asm("_binary_model_espdl_start");

// ====== Đối tượng cảm biến ======
MAX30105 particleSensor;
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
Adafruit_MPU6050 mpu;

// DS1302 RTC (constructor: RST, IO, CLK) — tùy lib có thể khác
DS1302 rtc(DS1302_RST, DS1302_IO, DS1302_CLK);

// ====== Biến đếm bước chân ======
static int stepCount = 0;
static float lastAccelMag = 0.0f;

// ======================== Sigmoid ========================
float sigmoid(float x) {
    return 1.0f / (1.0f + expf(-x));
}

// ======================== Chuẩn hóa =======================
const float means[NUM_FEATURES] = {79.53374663f, 36.74835291f, 97.50437243f, 53.44627537f, 0.49946505f, 74.99641903f, 1.75003102f};
const float stds[NUM_FEATURES]  = {11.55286498f, 0.43328918f, 1.44259433f, 20.78674961f, 0.49999971f, 14.4714659f,  0.14455348f};

// ======================== Hàm RTC ========================
String dayAsString(const Time::Day day) {
    switch (day) {
        case Time::kSunday:    return "Sunday";
        case Time::kMonday:    return "Monday";
        case Time::kTuesday:   return "Tuesday";
        case Time::kWednesday: return "Wednesday";
        case Time::kThursday:  return "Thursday";
        case Time::kFriday:    return "Friday";
        case Time::kSaturday:  return "Saturday";
    }
    return "(unknown day)";
}

void printTime() {
    // Get the current time and date from the chip
    Time t = rtc.time();

    const String day = dayAsString(t.day);

    char buf[50];
    snprintf(buf, sizeof(buf),
             "(%s) %04d-%02d-%02d %02d:%02d:%02d",
             day.c_str(), t.yr, t.mon, t.date,
             t.hr, t.min, t.sec);
    
    printf("⏰ Current RTC Time: %s\n", buf);
}

// ======================== Step counter (MPU6050 Adafruit) ========================
void update_step_count() {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    float ax = a.acceleration.x;
    float ay = a.acceleration.y;
    float az = a.acceleration.z;

    float accelMag = sqrtf(ax * ax + ay * ay + az * az);

    // simple peak detection threshold: điều chỉnh ngưỡng cho phù hợp
    const float THRESH = 1.0f; // ngưỡng thay đổi (m/s^2)
    if ((accelMag - lastAccelMag) > THRESH) {
        stepCount++;
    }
    lastAccelMag = accelMag;
}

// ======================== Hàm đọc cảm biến =======================
bool get_sensor_input_data(float input_data[NUM_SAMPLES][NUM_FEATURES]) {
    static uint32_t irBuffer[100];
    static uint32_t redBuffer[100];
    int32_t bufferLength = 100;

    // đọc MAX30102 (blocking read mẫu)
    for (int i = 0; i < bufferLength; i++) {
        // chờ mẫu mới
        while (!particleSensor.check()) {
            // tránh busy spin quá nhanh
            vTaskDelay(pdMS_TO_TICKS(2));
        }
        redBuffer[i] = particleSensor.getRed();
        irBuffer[i]  = particleSensor.getIR();
    }

    int32_t spo2 = 0;
    int8_t validSPO2 = 0;
    int32_t heartRate = 0;
    int8_t validHeartRate = 0;

    maxim_heart_rate_and_oxygen_saturation(
        irBuffer, bufferLength, redBuffer,
        &spo2, &validSPO2, &heartRate, &validHeartRate
    );

    float bodyTemp = mlx.readObjectTempC();

    // kiểm tra hợp lệ; khi in, ép kiểu về int để tránh cảnh báo format
    if ((!validSPO2 || !validHeartRate) 
        || (spo2 <= 94 || spo2 >= 100) 
        || (heartRate <= 50 || heartRate >= 130)) 
    {
        printf(" - Heart Rate: %d bpm\n", (int)heartRate);
        printf(" - SpO2: %d%%\n", (int)spo2);
        return false;
    }

    // gán cho input model (dạng float)
    input_data[0][0] = (float)heartRate;     // Heart Rate
    input_data[0][1] = bodyTemp;             // Body Temperature (°C)
    input_data[0][2] = (float)spo2;          // SpO2
    input_data[0][3] = 22.0f;                // Age
    input_data[0][4] = 0.0f;                 // Gender: 0 = Nữ
    input_data[0][5] = 70.0f;                // Weight (kg)
    input_data[0][6] = 1.70f;                // Height (m)

    printf("\n[Dữ liệu cảm biến]\nHR = %.1f bpm | SpO2 = %.1f%% | Temp = %.2f °C\n",
           input_data[0][0], input_data[0][2], input_data[0][1]);
    return true;
}

// ======================== Hàm chạy mô hình =======================
void run_hvsd_model(float input_data[NUM_SAMPLES][NUM_FEATURES]) {
    dl::Model *model = new dl::Model((const char *)model_espdl, fbs::MODEL_LOCATION_IN_FLASH_RODATA);
    ESP_ERROR_CHECK(model->test());

    dl::TensorBase *model_input = model->get_inputs().begin()->second;
    dl::TensorBase *model_output = model->get_outputs().begin()->second;

    // chuẩn hóa
    float input_scaled[NUM_SAMPLES][NUM_FEATURES];
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        for (int j = 0; j < NUM_FEATURES; ++j) {
            input_scaled[i][j] = (input_data[i][j] - means[j]) / stds[j];
        }
    }

    dl::TensorBase* input_tensor = new dl::TensorBase({NUM_SAMPLES, NUM_FEATURES}, nullptr, 0, dl::DATA_TYPE_FLOAT);
    memcpy(input_tensor->data, input_scaled, sizeof(input_scaled));
    model_input->assign(input_tensor);

    model->run();

    float* raw_output_data = (float*)model_output->data;
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        float logit = raw_output_data[i];
        float prob = sigmoid(logit);
        int class_label = (prob >= 0.5f) ? 1 : 0;
        const char* risk = (class_label == 1) ? "Low Risk" : "High Risk";

        printf("\n===== Kết quả suy luận AI =====\n");
        printf("HR=%.1f | Temp=%.2f | SpO2=%.2f | Age=%.0f | Gender=%s | W=%.1f | H=%.2f\n",
               input_data[i][0], input_data[i][1], input_data[i][2], input_data[i][3],
               (input_data[i][4] == 0.0f ? "Female" : "Male"),
               input_data[i][5], input_data[i][6]);
        printf("=> Logit=%.4f | Prob=%.4f -> %s\n", logit, prob, risk);
    }

    delete input_tensor;
    delete model;
}

// ======================== Setup cảm biến =======================
void setup_sensors() {
    // Khởi tạo I2C (dùng chân SDA=9, SCL=8 theo code trước; chỉnh nếu bạn dùng chân khác)
    Wire.begin(9, 8, 400000);

    // MAX30102
    if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
        printf("❌ Không tìm thấy MAX30102!\n");
        while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }
    particleSensor.setup(80, 4, 2, 100, 411, 4096);

    // MLX90614
    if (!mlx.begin()) {
        printf("❌ Không tìm thấy MLX90614!\n");
        while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    // MPU6050 (Adafruit)
    if (!mpu.begin()) {
        printf("❌ Không tìm thấy MPU6050 (Adafruit)!\n");
        while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    } else {
        // cấu hình sensor (tuỳ chọn)
        mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
        mpu.setGyroRange(MPU6050_RANGE_500_DEG);
        mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    }

    // DS1302 RTC
    rtc.halt(false);
    rtc.writeProtect(false);
    // Nếu RTC chưa có giờ đúng, set ban đầu (chỉ set 1 lần khi cần)
    // Lưu ý: một số lib yêu cầu năm là 2 chữ số; nếu thấy bị lệch thì chuyển yr = year - 2000
    Time t(2025, 10, 19, 8, 30, 50, Time::kSunday);
    rtc.time(t);
    // rtc.writeProtect(true);

    printf("✅ Tất cả cảm biến & RTC đã sẵn sàng.\n");
}

// ======================== app_main =======================
extern "C" void app_main(void) {
    setup_sensors();
    while (true) {
        // Cập nhật bước chân từ MPU
        update_step_count();
        printTime();
        float input_data[NUM_SAMPLES][NUM_FEATURES];
        if (get_sensor_input_data(input_data)) {
            run_hvsd_model(input_data);
        } else {
            printf("⚠️  Không có dữ liệu hợp lệ, thử lại...\n");
        }

        // In thời gian hiện tại và bước chân
        printf("👣 Step Count: %d\n", stepCount);

        // delay 1000 ms
        vTaskDelay(pdMS_TO_TICKS(1000));
        printf("-------------------------------------------\n");
    }
}
*/

/* Part 4: Tạo task riêng để in thời gian RTC mỗi giây, và xóa printTime() khỏi vòng lặp chính*/
/*
#include "dl_model_base.hpp"
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <unistd.h>

#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include "Adafruit_MLX90614.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include "DS1302.h"

// FreeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ====== Config ======
#define NUM_SAMPLES 1
#define NUM_FEATURES 7

// DS1302 pin
#define DS1302_RST   2
#define DS1302_IO    3
#define DS1302_CLK   4

extern const uint8_t model_espdl[] asm("_binary_model_espdl_start");

// ====== Sensors ======
MAX30105 particleSensor;
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
Adafruit_MPU6050 mpu;
DS1302 rtc(DS1302_RST, DS1302_IO, DS1302_CLK);

// ====== Step Counter ======
static int stepCount = 0;
static float lastAccelMag = 0.0f;

// ================= Sigmoid =================
float sigmoid(float x) {
    return 1.0f / (1.0f + expf(-x));
}

// ================= Normalization =================
const float means[NUM_FEATURES] = {79.53374663f, 36.74835291f, 97.50437243f, 53.44627537f, 0.49946505f, 74.99641903f, 1.75003102f};
const float stds[NUM_FEATURES]  = {11.55286498f, 0.43328918f, 1.44259433f, 20.78674961f, 0.49999971f, 14.4714659f,  0.14455348f};

// ================= RTC Functions =================
String dayAsString(const Time::Day day) {
    switch (day) {
        case Time::kSunday:    return "Sunday";
        case Time::kMonday:    return "Monday";
        case Time::kTuesday:   return "Tuesday";
        case Time::kWednesday: return "Wednesday";
        case Time::kThursday:  return "Thursday";
        case Time::kFriday:    return "Friday";
        case Time::kSaturday:  return "Saturday";
    }
    return "(unknown day)";
}

void printTime() {
    Time t = rtc.time();
    const String day = dayAsString(t.day);

    char buf[50];
    snprintf(buf, sizeof(buf),
             "(%s) %04d-%02d-%02d %02d:%02d:%02d",
             day.c_str(), t.yr, t.mon, t.date,
             t.hr, t.min, t.sec);

    printf("⏰ Current RTC Time: %s\n", buf);
}

// ✅ Task in RTC mỗi giây (tách khỏi loop chính)
void task_rtc_print(void *pvParameters) {
    while (true) {
        printTime();
        vTaskDelay(pdMS_TO_TICKS(1000)); // 1 giây
    }
}

// ================= Step Counter =================
void update_step_count() {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    float ax = a.acceleration.x;
    float ay = a.acceleration.y;
    float az = a.acceleration.z;
    float accelMag = sqrtf(ax * ax + ay * ay + az * az);

    const float THRESH = 1.0f;
    if ((accelMag - lastAccelMag) > THRESH) {
        stepCount++;
    }
    lastAccelMag = accelMag;
}

// ================= Sensor Input =================
bool get_sensor_input_data(float input_data[NUM_SAMPLES][NUM_FEATURES]) {
    static uint32_t irBuffer[100];
    static uint32_t redBuffer[100];
    int32_t bufferLength = 100;

    for (int i = 0; i < bufferLength; i++) {
        while (!particleSensor.check()) {
            vTaskDelay(pdMS_TO_TICKS(2));
        }
        redBuffer[i] = particleSensor.getRed();
        irBuffer[i]  = particleSensor.getIR();
    }

    int32_t spo2 = 0;
    int8_t validSPO2 = 0;
    int32_t heartRate = 0;
    int8_t validHeartRate = 0;

    maxim_heart_rate_and_oxygen_saturation(
        irBuffer, bufferLength, redBuffer,
        &spo2, &validSPO2, &heartRate, &validHeartRate
    );

    float bodyTemp = mlx.readObjectTempC();

    if ((!validSPO2 || !validHeartRate)
        || (spo2 <= 94 || spo2 >= 100)
        || (heartRate <= 50 || heartRate >= 130)) 
    {
        printf(" - Heart Rate: %d bpm\n", (int)heartRate);
        printf(" - SpO2: %d%%\n", (int)spo2);
        return false;
    }

    input_data[0][0] = (float)heartRate;
    input_data[0][1] = bodyTemp;
    input_data[0][2] = (float)spo2;
    input_data[0][3] = 22.0f;
    input_data[0][4] = 0.0f;
    input_data[0][5] = 70.0f;
    input_data[0][6] = 1.70f;

    printf("\n[Dữ liệu cảm biến]\nHR = %.1f bpm | SpO2 = %.1f%% | Temp = %.2f °C\n",
           input_data[0][0], input_data[0][2], input_data[0][1]);
    return true;
}

// ================= AI Inference =================
void run_hvsd_model(float input_data[NUM_SAMPLES][NUM_FEATURES]) {
    dl::Model *model = new dl::Model((const char *)model_espdl, fbs::MODEL_LOCATION_IN_FLASH_RODATA);
    ESP_ERROR_CHECK(model->test());

    dl::TensorBase *model_input = model->get_inputs().begin()->second;
    dl::TensorBase *model_output = model->get_outputs().begin()->second;

    float input_scaled[NUM_SAMPLES][NUM_FEATURES];
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        for (int j = 0; j < NUM_FEATURES; ++j) {
            input_scaled[i][j] = (input_data[i][j] - means[j]) / stds[j];
        }
    }

    dl::TensorBase* input_tensor = new dl::TensorBase({NUM_SAMPLES, NUM_FEATURES}, nullptr, 0, dl::DATA_TYPE_FLOAT);
    memcpy(input_tensor->data, input_scaled, sizeof(input_scaled));
    model_input->assign(input_tensor);

    model->run();

    float* raw_output_data = (float*)model_output->data;
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        float logit = raw_output_data[i];
        float prob = sigmoid(logit);
        int class_label = (prob >= 0.5f) ? 1 : 0;
        const char* risk = (class_label == 1) ? "Low Risk" : "High Risk";

        printf("\n===== Kết quả suy luận AI =====\n");
        printf("HR=%.1f | Temp=%.2f | SpO2=%.2f | Age=%.0f | Gender=%s | W=%.1f | H=%.2f\n",
               input_data[i][0], input_data[i][1], input_data[i][2], input_data[i][3],
               (input_data[i][4] == 0.0f ? "Female" : "Male"),
               input_data[i][5], input_data[i][6]);
        printf("=> Logit=%.4f | Prob=%.4f -> %s\n", logit, prob, risk);
    }

    delete input_tensor;
    delete model;
}

// ================= Setup Sensors & RTC =================
void setup_sensors() {
    Wire.begin(9, 8, 400000);

    if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
        printf("❌ Không tìm thấy MAX30102!\n");
        while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }
    particleSensor.setup(80, 4, 2, 100, 411, 4096);

    if (!mlx.begin()) {
        printf("❌ Không tìm thấy MLX90614!\n");
        while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    if (!mpu.begin()) {
        printf("❌ Không tìm thấy MPU6050!\n");
        while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    } else {
        mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
        mpu.setGyroRange(MPU6050_RANGE_500_DEG);
        mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    }

    rtc.halt(false);
    rtc.writeProtect(false);
    Time t(2025, 10, 19, 8, 30, 50, Time::kSunday);
    rtc.time(t);

    printf("✅ Tất cả cảm biến & RTC đã sẵn sàng.\n");
}

// ================= MAIN =================
extern "C" void app_main(void) {
    setup_sensors();

    // ✅ Tạo task in RTC riêng
    xTaskCreate(
        task_rtc_print,
        "RTC Task",
        4096,
        NULL,
        1,
        NULL
    );

    while (true) {
        update_step_count();

        float input_data[NUM_SAMPLES][NUM_FEATURES];
        if (get_sensor_input_data(input_data)) {
            run_hvsd_model(input_data);
        } else {
            printf("⚠️  Không có dữ liệu hợp lệ, thử lại...\n");
        }

        printf("👣 Step Count: %d\n", stepCount);

        vTaskDelay(pdMS_TO_TICKS(1000));
        printf("-------------------------------------------\n");
    }
}
*/

/* Part 5 - Kết hợp hiển thị màn hình tĩnh (Static UI)*/
/*
#include "dl_model_base.hpp"
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <unistd.h>

#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include "Adafruit_MLX90614.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <TFT_eSPI.h>
#include "DS1302.h"

// FreeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ====== Config ======
#define NUM_SAMPLES 1
#define NUM_FEATURES 7

// DS1302 pin
#define DS1302_RST   2
#define DS1302_IO    3
#define DS1302_CLK   4

extern const uint8_t model_espdl[] asm("_binary_model_espdl_start");

// ====== Display ======
TFT_eSPI tft = TFT_eSPI();
bool screenOn = true;

// ====== Sensors ======
MAX30105 particleSensor;
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
Adafruit_MPU6050 mpu;
DS1302 rtc(DS1302_RST, DS1302_IO, DS1302_CLK);

// ====== Step Counter ======
static int stepCount = 0;
static float lastAccelMag = 0.0f;

// ================= Sigmoid =================
float sigmoid(float x) {
    return 1.0f / (1.0f + expf(-x));
}

// ================= Normalization =================
const float means[NUM_FEATURES] = {79.53374663f, 36.74835291f, 97.50437243f, 53.44627537f, 0.49946505f, 74.99641903f, 1.75003102f};
const float stds[NUM_FEATURES]  = {11.55286498f, 0.43328918f, 1.44259433f, 20.78674961f, 0.49999971f, 14.4714659f,  0.14455348f};

// ================= RTC Functions =================
String dayAsString(const Time::Day day) {
    switch (day) {
        case Time::kSunday:    return "Sunday";
        case Time::kMonday:    return "Monday";
        case Time::kTuesday:   return "Tuesday";
        case Time::kWednesday: return "Wednesday";
        case Time::kThursday:  return "Thursday";
        case Time::kFriday:    return "Friday";
        case Time::kSaturday:  return "Saturday";
    }
    return "(unknown day)";
}

void printTime() {
    Time t = rtc.time();
    const String day = dayAsString(t.day);

    char buf[50];
    snprintf(buf, sizeof(buf),
             "(%s) %04d-%02d-%02d %02d:%02d:%02d",
             day.c_str(), t.yr, t.mon, t.date,
             t.hr, t.min, t.sec);

    printf("⏰ Current RTC Time: %s\n", buf);
}

// ✅ Task in RTC mỗi giây (tách khỏi loop chính)
void task_rtc_print(void *pvParameters) {
    while (true) {
        printTime();
        vTaskDelay(pdMS_TO_TICKS(1000)); // 1 giây
    }
}

// ================= Step Counter =================
void update_step_count() {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    float ax = a.acceleration.x;
    float ay = a.acceleration.y;
    float az = a.acceleration.z;
    float accelMag = sqrtf(ax * ax + ay * ay + az * az);

    const float THRESH = 1.0f;
    if ((accelMag - lastAccelMag) > THRESH) {
        stepCount++;
    }
    lastAccelMag = accelMag;
}

// ================= Sensor Input =================
bool get_sensor_input_data(float input_data[NUM_SAMPLES][NUM_FEATURES]) {
    static uint32_t irBuffer[100];
    static uint32_t redBuffer[100];
    int32_t bufferLength = 100;

    for (int i = 0; i < bufferLength; i++) {
        while (!particleSensor.check()) {
            vTaskDelay(pdMS_TO_TICKS(2));
        }
        redBuffer[i] = particleSensor.getRed();
        irBuffer[i]  = particleSensor.getIR();
    }

    int32_t spo2 = 0;
    int8_t validSPO2 = 0;
    int32_t heartRate = 0;
    int8_t validHeartRate = 0;

    maxim_heart_rate_and_oxygen_saturation(
        irBuffer, bufferLength, redBuffer,
        &spo2, &validSPO2, &heartRate, &validHeartRate
    );

    float bodyTemp = mlx.readObjectTempC();

    if ((!validSPO2 || !validHeartRate)
        || (spo2 <= 94 || spo2 >= 100)
        || (heartRate <= 50 || heartRate >= 130)) 
    {
        printf(" - Heart Rate: %d bpm\n", (int)heartRate);
        printf(" - SpO2: %d%%\n", (int)spo2);
        return false;
    }

    input_data[0][0] = (float)heartRate;
    input_data[0][1] = bodyTemp;
    input_data[0][2] = (float)spo2;
    input_data[0][3] = 22.0f;
    input_data[0][4] = 0.0f;
    input_data[0][5] = 70.0f;
    input_data[0][6] = 1.70f;

    printf("\n[Dữ liệu cảm biến]\nHR = %.1f bpm | SpO2 = %.1f%% | Temp = %.2f °C\n",
           input_data[0][0], input_data[0][2], input_data[0][1]);
    return true;
}

// ================= AI Inference =================
void run_hvsd_model(float input_data[NUM_SAMPLES][NUM_FEATURES]) {
    dl::Model *model = new dl::Model((const char *)model_espdl, fbs::MODEL_LOCATION_IN_FLASH_RODATA);
    ESP_ERROR_CHECK(model->test());

    dl::TensorBase *model_input = model->get_inputs().begin()->second;
    dl::TensorBase *model_output = model->get_outputs().begin()->second;

    float input_scaled[NUM_SAMPLES][NUM_FEATURES];
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        for (int j = 0; j < NUM_FEATURES; ++j) {
            input_scaled[i][j] = (input_data[i][j] - means[j]) / stds[j];
        }
    }

    dl::TensorBase* input_tensor = new dl::TensorBase({NUM_SAMPLES, NUM_FEATURES}, nullptr, 0, dl::DATA_TYPE_FLOAT);
    memcpy(input_tensor->data, input_scaled, sizeof(input_scaled));
    model_input->assign(input_tensor);

    model->run();

    float* raw_output_data = (float*)model_output->data;
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        float logit = raw_output_data[i];
        float prob = sigmoid(logit);
        int class_label = (prob >= 0.5f) ? 1 : 0;
        const char* risk = (class_label == 1) ? "Low Risk" : "High Risk";

        printf("\n===== Kết quả suy luận AI =====\n");
        printf("HR=%.1f | Temp=%.2f | SpO2=%.2f | Age=%.0f | Gender=%s | W=%.1f | H=%.2f\n",
               input_data[i][0], input_data[i][1], input_data[i][2], input_data[i][3],
               (input_data[i][4] == 0.0f ? "Female" : "Male"),
               input_data[i][5], input_data[i][6]);
        printf("=> Logit=%.4f | Prob=%.4f -> %s\n", logit, prob, risk);
    
        // Hiển thị kết quả AI trên TFT
        display_ai_result(risk);
    }

    delete input_tensor;
    delete model;
}

// ================= Draw Statuc UI =================
void drawInterface() {
    tft.fillScreen(TFT_BLACK);
    tft.fillRoundRect(0, 0, 240, 40, 5, TFT_BLUE);
    tft.setTextColor(TFT_WHITE, TFT_BLUE);
    tft.setTextSize(2);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("HEALTH MONITOR", 120, 15);

    tft.setTextSize(2);
    tft.drawRoundRect(5, 45, 230, 70, 5, TFT_WHITE);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, 50); tft.print("TIME");

    tft.drawRoundRect(5, 120, 110, 60, 5, TFT_WHITE);
    tft.setCursor(10, 125); tft.print("HEART");
    tft.drawRoundRect(125, 120, 110, 60, 5, TFT_WHITE);
    tft.setCursor(130, 125); tft.print("TEMP");
    tft.drawRoundRect(5, 190, 110, 60, 5, TFT_WHITE);
    tft.setCursor(10, 195); tft.print("SPO2");
    tft.drawRoundRect(125, 190, 110, 60, 5, TFT_WHITE);
    tft.setCursor(130, 195); tft.print("STEPS");

    tft.drawRoundRect(5, 260, 230, 55, 5, TFT_WHITE);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, 265); tft.print("AI RISK:");
}

// ================= Display AI on TFT ==================
void display_ai_result(const char* risk) {
    // Xóa nội dung cũ trong khung
    tft.fillRoundRect(5, 260, 230, 55, 5, TFT_BLACK);
    tft.drawRoundRect(5, 260, 230, 55, 5, TFT_WHITE);
    tft.setTextSize(2);

    // Đổi màu chữ theo nguy cơ
    if (strcmp(risk, "High Risk") == 0) {
        tft.setTextColor(TFT_RED, TFT_BLACK);
    } else {
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
    }

    tft.setCursor(15, 280);
    tft.print(risk);
}

// ================= Setup Sensors & RTC =================
void setup_sensors() {
    Wire.begin(9, 8, 400000);

    if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
        printf("❌ Không tìm thấy MAX30102!\n");
        while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }
    particleSensor.setup(80, 4, 2, 100, 411, 4096);

    if (!mlx.begin()) {
        printf("❌ Không tìm thấy MLX90614!\n");
        while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    if (!mpu.begin()) {
        printf("❌ Không tìm thấy MPU6050!\n");
        while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    } else {
        mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
        mpu.setGyroRange(MPU6050_RANGE_500_DEG);
        mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    }

    rtc.halt(false);
    rtc.writeProtect(false);
    Time t(2025, 10, 19, 8, 30, 50, Time::kSunday);
    rtc.time(t);

    printf("✅ Tất cả cảm biến & RTC đã sẵn sàng.\n");

    tft.init();
    tft.setRotation(0);
    tft.setTextSize(1);
    drawInterface();
}

// ================= MAIN =================
extern "C" void app_main(void) {
    setup_sensors();

    // ✅ Tạo task in RTC riêng
    xTaskCreate(
        task_rtc_print,
        "RTC Task",
        4096,
        NULL,
        1,
        NULL
    );

    while (true) {
        update_step_count();

        float input_data[NUM_SAMPLES][NUM_FEATURES];
        if (get_sensor_input_data(input_data)) {
            run_hvsd_model(input_data);
        } else {
            printf("⚠️  Không có dữ liệu hợp lệ, thử lại...\n");
        }

        printf("👣 Step Count: %d\n", stepCount);

        vTaskDelay(pdMS_TO_TICKS(1000));
        printf("-------------------------------------------\n");
    }
}
*/

/* Part 6: Full Code AIoT - chưa có FREERTOS */
/*
#include "dl_model_base.hpp"
#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include "Adafruit_MLX90614.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <TFT_eSPI.h>
#include "DS1302.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ====== Config ======
#define NUM_SAMPLES 1
#define NUM_FEATURES 7

// DS1302 pin
#define DS1302_RST   2
#define DS1302_IO    3
#define DS1302_CLK   4

extern const uint8_t model_espdl[] asm("_binary_model_espdl_start");

// ====== Display ======
TFT_eSPI tft = TFT_eSPI();
bool screenOn = true;

// ====== Sensors ======
MAX30105 particleSensor;
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
Adafruit_MPU6050 mpu;
DS1302 rtc(DS1302_RST, DS1302_IO, DS1302_CLK);

// ====== Step Counter ======
static int stepCount = 0;
static float lastAccelMag = 0.0f;

// ================= Sigmoid =================
float sigmoid(float x) {
    return 1.0f / (1.0f + expf(-x));
}

// ================= Normalization =================
const float means[NUM_FEATURES] = {79.53374663f, 36.74835291f, 97.50437243f, 53.44627537f, 0.49946505f, 74.99641903f, 1.75003102f};
const float stds[NUM_FEATURES]  = {11.55286498f, 0.43328918f, 1.44259433f, 20.78674961f, 0.49999971f, 14.4714659f,  0.14455348f};

// ================= Forward declaration =================
void display_ai_result(const char* risk);
void update_sensor_display(int hr, float temp, int spo2, int steps);

// ================= RTC Functions =================
String dayAsString(const Time::Day day) {
    switch (day) {
        case Time::kSunday:    return "Sunday";
        case Time::kMonday:    return "Monday";
        case Time::kTuesday:   return "Tuesday";
        case Time::kWednesday: return "Wednesday";
        case Time::kThursday:  return "Thursday";
        case Time::kFriday:    return "Friday";
        case Time::kSaturday:  return "Saturday";
    }
    return "(unknown day)";
}

void update_time_display() {
    Time t = rtc.time();
    const String day = dayAsString(t.day);

    char timeBuf[40];
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", t.hr, t.min, t.sec);
    tft.setTextDatum(TL_DATUM); tft.setTextSize(2);
    tft.fillRect(10,65,220,25,TFT_BLACK); tft.fillRect(10,90,220,25,TFT_BLACK);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString(timeBuf,15,70);
}

// ✅ Task in RTC mỗi giây (tách khỏi loop chính)
void task_rtc_display(void *pvParameters) {
    while (true) {
        update_time_display();
        vTaskDelay(pdMS_TO_TICKS(1000)); // 1 giây
    }
}

// ================= Step Counter =================
void update_step_count(float ax, float ay, float az) {
    float accelMag = sqrtf(ax * ax + ay * ay + az * az);
    const float THRESH = 1.0f;
    if ((accelMag - lastAccelMag) > THRESH) {
        stepCount++;
    }
    lastAccelMag = accelMag;
}

// ================= Sensor Input =================
bool get_sensor_input_data(float input_data[NUM_SAMPLES][NUM_FEATURES]) {
    static uint32_t irBuffer[100];
    static uint32_t redBuffer[100];
    int32_t bufferLength = 100;

    for (int i = 0; i < bufferLength; i++) {
        while (!particleSensor.check()) {
            vTaskDelay(pdMS_TO_TICKS(2));
        }
        redBuffer[i] = particleSensor.getRed();
        irBuffer[i]  = particleSensor.getIR();
    }

    int32_t spo2 = 0;
    int8_t validSPO2 = 0;
    int32_t heartRate = 0;
    int8_t validHeartRate = 0;

    maxim_heart_rate_and_oxygen_saturation(
        irBuffer, bufferLength, redBuffer,
        &spo2, &validSPO2, &heartRate, &validHeartRate
    );

    float bodyTemp = mlx.readObjectTempC();

    if ((!validSPO2 || !validHeartRate)
        || (spo2 <= 94 || spo2 >= 100)
        || (heartRate <= 50 || heartRate >= 130)) 
    {
        printf(" - Heart Rate: %d bpm\n", (int)heartRate);
        printf(" - SpO2: %d%%\n", (int)spo2);
        return false;
    }

    input_data[0][0] = (int)heartRate;
    input_data[0][1] = bodyTemp;
    input_data[0][2] = (int)spo2;
    input_data[0][3] = 22.0f;   // Age
    input_data[0][4] = 0.0f;    // Gender: 0 Female, 1 Male
    input_data[0][5] = 70.0f;   // Weight
    input_data[0][6] = 1.7f;    // Height

    update_sensor_display(heartRate, bodyTemp, spo2, stepCount);
    return true;
}

// ================= Display Sensor =================
void update_sensor_display(int hr, float temp, int spo2, int steps) {
    // Heart Rate
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    char hrBuf[20]; sprintf(hrBuf,"%d BPM", hr);
    tft.fillRect(10,145,100,25,TFT_BLACK); tft.drawString(hrBuf,15,150);

    // Temp
    char tempBuf[20]; sprintf(tempBuf,"%.1f C", temp);
    if (temp > 37.8) tft.setTextColor(TFT_RED,TFT_BLACK);
    else if (temp > 37.0) tft.setTextColor(TFT_YELLOW,TFT_BLACK);
    else tft.setTextColor(TFT_WHITE,TFT_BLACK);
    tft.fillRect(130,145,100,30,TFT_BLACK); tft.drawString(tempBuf,135,150);


    // SpO2
    tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
    char spo2Buf[20]; sprintf(spo2Buf,"%d %%", spo2);
    tft.fillRect(10,210,100,25,TFT_BLACK); tft.drawString(spo2Buf,15,215);


    // Steps
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    char stepBuf[20]; sprintf(stepBuf,"%d", stepCount);
    tft.fillRect(130,210,100,25,TFT_BLACK); tft.drawString(stepBuf,135,215);
}

// ================= AI Inference =================
void run_hvsd_model(float input_data[NUM_SAMPLES][NUM_FEATURES]) {
    dl::Model *model = new dl::Model((const char *)model_espdl, fbs::MODEL_LOCATION_IN_FLASH_RODATA);
    ESP_ERROR_CHECK(model->test());

    dl::TensorBase *model_input = model->get_inputs().begin()->second;
    dl::TensorBase *model_output = model->get_outputs().begin()->second;

    float input_scaled[NUM_SAMPLES][NUM_FEATURES];
    for (int i = 0; i < NUM_SAMPLES; ++i)
        for (int j = 0; j < NUM_FEATURES; ++j)
            input_scaled[i][j] = (input_data[i][j] - means[j]) / stds[j];

    dl::TensorBase* input_tensor = new dl::TensorBase({NUM_SAMPLES, NUM_FEATURES}, nullptr, 0, dl::DATA_TYPE_FLOAT);
    memcpy(input_tensor->data, input_scaled, sizeof(input_scaled));
    model_input->assign(input_tensor);

    model->run();

    float* raw_output_data = (float*)model_output->data;
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        float logit = raw_output_data[i];
        float prob = sigmoid(logit);
        int class_label = (prob >= 0.5f) ? 1 : 0;
        const char* risk = (class_label == 1) ? "Low Risk" : "High Risk";

        printf("\n===== Kết quả AI =====\n");
        printf("HR=%.1f | Temp=%.2f | SpO2=%.2f | Risk=%s\n",
               input_data[i][0], input_data[i][1], input_data[i][2], risk);
    }

    delete input_tensor;
    delete model;
}

// ================= Draw UI =================
void drawInterface() {
    tft.fillScreen(TFT_BLACK);
    tft.fillRoundRect(0,0,240,40,8,TFT_BLUE);
    tft.setTextColor(TFT_WHITE, TFT_BLUE);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2);
    tft.drawString("HEALTH MONITOR",120,20);

    tft.setTextSize(2);
    tft.drawRoundRect(5,45,230,70,5,TFT_WHITE);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10,50); tft.print("TIME");

    tft.drawRoundRect(5,120,110,60,5,TFT_WHITE); tft.setCursor(10,125); tft.print("HEART");
    tft.drawRoundRect(125,120,110,60,5,TFT_WHITE); tft.setCursor(130,125); tft.print("TEMP");
    tft.drawRoundRect(5,190,110,60,5,TFT_WHITE); tft.setCursor(10,195); tft.print("SPO2");
    tft.drawRoundRect(125,190,110,60,5,TFT_WHITE); tft.setCursor(130,195); tft.print("STEPS");
}

// ================= Setup Sensors & RTC =================
void setup_sensors() {
    Wire.begin(9, 8, 400000);

    if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) { while (1) vTaskDelay(pdMS_TO_TICKS(1000)); }
    particleSensor.setup(80, 4, 2, 100, 411, 4096);

    if (!mlx.begin()) { while (1) vTaskDelay(pdMS_TO_TICKS(1000)); }

    if (!mpu.begin()) { while (1) vTaskDelay(pdMS_TO_TICKS(1000)); } 
    else {
        mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
        mpu.setGyroRange(MPU6050_RANGE_500_DEG);
        mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    }

    rtc.halt(false);
    rtc.writeProtect(false);
    Time t(2025, 10, 19, 8, 30, 50, Time::kSunday);
    rtc.time(t);

    tft.init();
    tft.setRotation(0);
    tft.setTextSize(1);
    drawInterface();
}

// ================= MAIN =================
extern "C" void app_main(void) {
    setup_sensors();

    // Tạo task hiển thị RTC
    xTaskCreate(task_rtc_display, "RTC Display Task", 4096, NULL, 1, NULL);

    while (true) {
        sensors_event_t a, g, temp;
        mpu.getEvent(&a, &g, &temp);
        update_step_count(a.acceleration.x, a.acceleration.y, a.acceleration.z);

        float input_data[NUM_SAMPLES][NUM_FEATURES];
        if (get_sensor_input_data(input_data)) {
            run_hvsd_model(input_data);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
*/

/* Part 7: Full Code AIoT - Cải tiến thêm dùng RTOS hiển thị MLX90614 và Step riêng trên màn hình*/
/*
#include "dl_model_base.hpp"
#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include "Adafruit_MLX90614.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <TFT_eSPI.h>
#include "DS1302.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// ====== Config ======
#define NUM_SAMPLES 1
#define NUM_FEATURES 7

// DS1302 pin
#define DS1302_RST   2
#define DS1302_IO    3
#define DS1302_CLK   4

extern const uint8_t model_espdl[] asm("_binary_model_espdl_start");

// ====== Display ======
TFT_eSPI tft = TFT_eSPI();
SemaphoreHandle_t tftMutex;

// ====== Sensors ======
MAX30105 particleSensor;
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
Adafruit_MPU6050 mpu;
DS1302 rtc(DS1302_RST, DS1302_IO, DS1302_CLK);

// ====== Step Counter ======
static int stepCount = 0;

// ================= Sigmoid =================
float sigmoid(float x) {
    return 1.0f / (1.0f + expf(-x));
}

// ================= Normalization =================
const float means[NUM_FEATURES] = {79.53374663f, 36.74835291f, 97.50437243f, 53.44627537f, 0.49946505f, 74.99641903f, 1.75003102f};
const float stds[NUM_FEATURES]  = {11.55286498f, 0.43328918f, 1.44259433f, 20.78674961f, 0.49999971f, 14.4714659f,  0.14455348f};

// ================= Forward declaration =================
void update_sensor_display(int hr, int spo2);
void update_time_display();

// ================= RTC Functions =================
String dayAsString(const Time::Day day) {
    switch (day) {
        case Time::kSunday:    return "Sunday";
        case Time::kMonday:    return "Monday";
        case Time::kTuesday:   return "Tuesday";
        case Time::kWednesday: return "Wednesday";
        case Time::kThursday:  return "Thursday";
        case Time::kFriday:    return "Friday";
        case Time::kSaturday:  return "Saturday";
    }
    return "(unknown day)";
}

// ================= Sensor Input =================
bool get_sensor_input_data(float input_data[NUM_SAMPLES][NUM_FEATURES]) {
    static uint32_t irBuffer[100];
    static uint32_t redBuffer[100];
    int32_t bufferLength = 100;

    for (int i = 0; i < bufferLength; i++) {
        while (!particleSensor.check()) {
            vTaskDelay(pdMS_TO_TICKS(2));
        }
        redBuffer[i] = particleSensor.getRed();
        irBuffer[i]  = particleSensor.getIR();
    }

    int32_t spo2 = 0;
    int8_t validSPO2 = 0;
    int32_t heartRate = 0;
    int8_t validHeartRate = 0;

    maxim_heart_rate_and_oxygen_saturation(
        irBuffer, bufferLength, redBuffer,
        &spo2, &validSPO2, &heartRate, &validHeartRate
    );

    float bodyTemp = mlx.readObjectTempC();

    if ((!validSPO2 || !validHeartRate)
        || (spo2 <= 94 || spo2 >= 100)
        || (heartRate <= 50 || heartRate >= 130)) 
    {
        printf(" - Heart Rate: %d bpm\n", (int)heartRate);
        printf(" - SpO2: %d%%\n", (int)spo2);
        return false;
    }
    
    input_data[0][0] = (int)heartRate;
    input_data[0][1] = bodyTemp;
    input_data[0][2] = (int)spo2;
    input_data[0][3] = 22.0f;   // Age
    input_data[0][4] = 0.0f;    // Gender: 0 Female, 1 Male
    input_data[0][5] = 70.0f;   // Weight
    input_data[0][6] = 1.7f;    // Height

    update_sensor_display(heartRate, spo2);
    return true;
}

// ================= Display Sensor =================
void update_sensor_display(int hr, int spo2) {
    if (xSemaphoreTake(tftMutex, portMAX_DELAY)) {
        // Heart Rate
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        char hrBuf[20]; 
        sprintf(hrBuf, "%d BPM", hr);
        tft.fillRect(10, 145, 100, 25, TFT_BLACK);  // clear previous
        tft.drawString(hrBuf, 15, 150);

        // SpO2
        tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
        char spo2Buf[20]; 
        sprintf(spo2Buf, "%d %%", spo2);
        tft.fillRect(10, 210, 100, 25, TFT_BLACK);  // clear previous
        tft.drawString(spo2Buf, 15, 215);

        xSemaphoreGive(tftMutex);
    }
}

// ================= AI Inference =================
void run_hvsd_model(float input_data[NUM_SAMPLES][NUM_FEATURES]) {
    dl::Model *model = new dl::Model((const char *)model_espdl, fbs::MODEL_LOCATION_IN_FLASH_RODATA);
    ESP_ERROR_CHECK(model->test());

    dl::TensorBase *model_input = model->get_inputs().begin()->second;
    dl::TensorBase *model_output = model->get_outputs().begin()->second;

    float input_scaled[NUM_SAMPLES][NUM_FEATURES];
    for (int i = 0; i < NUM_SAMPLES; ++i)
        for (int j = 0; j < NUM_FEATURES; ++j)
            input_scaled[i][j] = (input_data[i][j] - means[j]) / stds[j];

    dl::TensorBase* input_tensor = new dl::TensorBase({NUM_SAMPLES, NUM_FEATURES}, nullptr, 0, dl::DATA_TYPE_FLOAT);
    memcpy(input_tensor->data, input_scaled, sizeof(input_scaled));
    model_input->assign(input_tensor);

    model->run();

    float* raw_output_data = (float*)model_output->data;
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        float logit = raw_output_data[i];
        float prob = sigmoid(logit);
        int class_label = (prob >= 0.5f) ? 1 : 0;
        const char* risk = (class_label == 1) ? "Low Risk" : "High Risk";

        printf("\n===== Kết quả AI =====\n");
        printf("HR=%.1f | Temp=%.2f | SpO2=%.2f | Risk=%s\n",
               input_data[i][0], input_data[i][1], input_data[i][2], risk);
    }

    delete input_tensor;
    delete model;
}

// ================= Draw UI =================
void drawInterface() {
    if(xSemaphoreTake(tftMutex, portMAX_DELAY)) {
        tft.fillScreen(TFT_BLACK);
        tft.fillRoundRect(0,0,240,40,8,TFT_BLUE);
        tft.setTextColor(TFT_WHITE, TFT_BLUE);
        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(2);
        tft.drawString("HEALTH MONITOR",120,20);

        tft.setTextSize(2);
        tft.drawRoundRect(5,45,230,70,5,TFT_WHITE);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setCursor(10,50); tft.print("TIME");

        tft.drawRoundRect(5,120,110,60,5,TFT_WHITE); tft.setCursor(10,125); tft.print("HEART");
        tft.drawRoundRect(125,120,110,60,5,TFT_WHITE); tft.setCursor(130,125); tft.print("TEMP");
        tft.drawRoundRect(5,190,110,60,5,TFT_WHITE); tft.setCursor(10,195); tft.print("SPO2");
        tft.drawRoundRect(125,190,110,60,5,TFT_WHITE); tft.setCursor(130,195); tft.print("STEPS");

        xSemaphoreGive(tftMutex);
    }
}

// ================= RTC Display Task =================
void task_rtc_display(void *pvParameters) {
    while (true) {
        update_time_display();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void update_time_display() {
    Time t = rtc.time();
    const String day = dayAsString(t.day);

    char timeBuf[40];
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", t.hr, t.min, t.sec);

    if(xSemaphoreTake(tftMutex, portMAX_DELAY)) {
        tft.setTextDatum(TL_DATUM); tft.setTextSize(2);
        tft.fillRect(10,65,220,25,TFT_BLACK);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.drawString(timeBuf,15,70);
        xSemaphoreGive(tftMutex);
    }
}

// ================= Step Counter Task =================
void task_step_counter(void *pvParameters) {
    static float lastAccelMag = 0.0f;

    while (true) {
        sensors_event_t a, g, temp_event;
        mpu.getEvent(&a, &g, &temp_event);

        float accelMag = sqrtf(a.acceleration.x*a.acceleration.x +
                               a.acceleration.y*a.acceleration.y +
                               a.acceleration.z*a.acceleration.z);
        const float THRESH = 2.0f;
        if ((accelMag - lastAccelMag) > THRESH) stepCount++;
        lastAccelMag = accelMag;

        if(xSemaphoreTake(tftMutex, portMAX_DELAY)) {
            tft.setTextColor(TFT_CYAN, TFT_BLACK);
            char stepBuf[20]; sprintf(stepBuf,"%d", stepCount);
            tft.fillRect(125,210,100,25,TFT_BLACK);
            tft.drawString(stepBuf,135,215);
            xSemaphoreGive(tftMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// ================= Temperature Task =================
void task_temp_display(void *pvParameters) {
    while (true) {
        float temp = mlx.readObjectTempC();

        if (xSemaphoreTake(tftMutex, portMAX_DELAY)) {
            // Chọn màu theo nhiệt độ
            if (temp > 37.8) tft.setTextColor(TFT_RED, TFT_BLACK);
            else if (temp > 37.0) tft.setTextColor(TFT_YELLOW, TFT_BLACK);
            else tft.setTextColor(TFT_WHITE, TFT_BLACK);

            char tempBuf[20];
            sprintf(tempBuf, "%.1f C", temp);

            // Xóa vùng cũ: tăng chiều rộng + chiều cao hơn font
            tft.fillRect(130, 145, 110, 40, TFT_BLACK); // tăng kích thước vùng
            tft.setTextDatum(TL_DATUM); // gốc trên-trái
            tft.setTextSize(2);          // chắc chắn font size = 2
            tft.drawString(tempBuf, 135, 150);

            xSemaphoreGive(tftMutex);
        }


        vTaskDelay(pdMS_TO_TICKS(500)); // update 0.5s
    }
}

// ================= Setup Sensors & RTC =================
void setup_sensors() {
    Wire.begin(9, 8, 400000);

    if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) { while (1) vTaskDelay(pdMS_TO_TICKS(1000)); }
    particleSensor.setup(80, 4, 2, 100, 411, 4096);

    if (!mlx.begin()) { while (1) vTaskDelay(pdMS_TO_TICKS(1000)); }

    if (!mpu.begin()) { while (1) vTaskDelay(pdMS_TO_TICKS(1000)); } 
    else {
        mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
        mpu.setGyroRange(MPU6050_RANGE_500_DEG);
        mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    }

    rtc.halt(false);
    rtc.writeProtect(false);
    Time t(2025, 10, 19, 8, 30, 50, Time::kSunday);
    rtc.time(t);

    tft.init();
    tft.setRotation(0);

    // Tạo mutex TFT
    tftMutex = xSemaphoreCreateMutex();
    if(!tftMutex) while(1);

    drawInterface();
}

// ================= MAIN =================
extern "C" void app_main(void) {
    setup_sensors();

    // Tạo tasks
    xTaskCreate(task_rtc_display, "RTC Display Task", 4096, NULL, 1, NULL);
    xTaskCreate(task_step_counter, "Step Counter Task", 4096, NULL, 1, NULL);
    xTaskCreate(task_temp_display, "Temp Display Task", 4096, NULL, 1, NULL);

    while (true) {
        float input_data[NUM_SAMPLES][NUM_FEATURES];
        if (get_sensor_input_data(input_data)) {
            run_hvsd_model(input_data);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
*/

/* Part 8: Full Code AIOT + Deep Sleep mode*/
/*
#include "dl_model_base.hpp"
#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include "Adafruit_MLX90614.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <TFT_eSPI.h>
#include "DS1302.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_sleep.h"

// ====== Config ======
#define NUM_SAMPLES 1
#define NUM_FEATURES 7

// DS1302 pin
#define DS1302_RST   2
#define DS1302_IO    3
#define DS1302_CLK   4

// Button for Deep Sleep
#define BUTTON_DEEP_SLEEP 1
volatile bool deepSleepRequested = false;

extern const uint8_t model_espdl[] asm("_binary_model_espdl_start");

// ====== Display ======
TFT_eSPI tft = TFT_eSPI();
SemaphoreHandle_t tftMutex;

// ====== Sensors ======
MAX30105 particleSensor;
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
Adafruit_MPU6050 mpu;
DS1302 rtc(DS1302_RST, DS1302_IO, DS1302_CLK);

// ====== Step Counter ======
static int stepCount = 0;

// ================= Sigmoid =================
float sigmoid(float x) {
    return 1.0f / (1.0f + expf(-x));
}

// ================= Normalization =================
const float means[NUM_FEATURES] = {79.53374663f, 36.74835291f, 97.50437243f, 53.44627537f, 0.49946505f, 74.99641903f, 1.75003102f};
const float stds[NUM_FEATURES]  = {11.55286498f, 0.43328918f, 1.44259433f, 20.78674961f, 0.49999971f, 14.4714659f,  0.14455348f};

// ================= Forward declaration =================
void update_sensor_display(int hr, int spo2);
void update_time_display();

// ================= RTC Functions =================
String dayAsString(const Time::Day day) {
    switch (day) {
        case Time::kSunday:    return "Sunday";
        case Time::kMonday:    return "Monday";
        case Time::kTuesday:   return "Tuesday";
        case Time::kWednesday: return "Wednesday";
        case Time::kThursday:  return "Thursday";
        case Time::kFriday:    return "Friday";
        case Time::kSaturday:  return "Saturday";
    }
    return "(unknown day)";
}

// ================= Sensor Input =================
bool get_sensor_input_data(float input_data[NUM_SAMPLES][NUM_FEATURES]) {
    static uint32_t irBuffer[100];
    static uint32_t redBuffer[100];
    int32_t bufferLength = 100;

    for (int i = 0; i < bufferLength; i++) {
        while (!particleSensor.check()) {
            vTaskDelay(pdMS_TO_TICKS(2));
        }
        redBuffer[i] = particleSensor.getRed();
        irBuffer[i]  = particleSensor.getIR();
    }

    int32_t spo2 = 0;
    int8_t validSPO2 = 0;
    int32_t heartRate = 0;
    int8_t validHeartRate = 0;

    maxim_heart_rate_and_oxygen_saturation(
        irBuffer, bufferLength, redBuffer,
        &spo2, &validSPO2, &heartRate, &validHeartRate
    );

    float bodyTemp = mlx.readObjectTempC();

    if ((!validSPO2 || !validHeartRate)
        || (spo2 <= 94 || spo2 >= 100)
        || (heartRate <= 50 || heartRate >= 130)) 
    {
        printf(" - Heart Rate: %d bpm\n", (int)heartRate);
        printf(" - SpO2: %d%%\n", (int)spo2);
        return false;
    }
    
    input_data[0][0] = (int)heartRate;
    input_data[0][1] = bodyTemp;
    input_data[0][2] = (int)spo2;
    input_data[0][3] = 22.0f;   // Age
    input_data[0][4] = 0.0f;    // Gender: 0 Female, 1 Male
    input_data[0][5] = 70.0f;   // Weight
    input_data[0][6] = 1.7f;    // Height

    update_sensor_display(heartRate, spo2);
    return true;
}

// ================= Display Sensor =================
void update_sensor_display(int hr, int spo2) {
    if (xSemaphoreTake(tftMutex, portMAX_DELAY)) {
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        char hrBuf[20]; 
        sprintf(hrBuf, "%d BPM", hr);
        tft.fillRect(10, 145, 100, 25, TFT_BLACK);
        tft.drawString(hrBuf, 15, 150);

        tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
        char spo2Buf[20]; 
        sprintf(spo2Buf, "%d %%", spo2);
        tft.fillRect(10, 210, 100, 25, TFT_BLACK);
        tft.drawString(spo2Buf, 15, 215);

        xSemaphoreGive(tftMutex);
    }
}

// ================= AI Inference =================
void run_hvsd_model(float input_data[NUM_SAMPLES][NUM_FEATURES]) {
    dl::Model *model = new dl::Model((const char *)model_espdl, fbs::MODEL_LOCATION_IN_FLASH_RODATA);
    ESP_ERROR_CHECK(model->test());

    dl::TensorBase *model_input = model->get_inputs().begin()->second;
    dl::TensorBase *model_output = model->get_outputs().begin()->second;

    float input_scaled[NUM_SAMPLES][NUM_FEATURES];
    for (int i = 0; i < NUM_SAMPLES; ++i)
        for (int j = 0; j < NUM_FEATURES; ++j)
            input_scaled[i][j] = (input_data[i][j] - means[j]) / stds[j];

    dl::TensorBase* input_tensor = new dl::TensorBase({NUM_SAMPLES, NUM_FEATURES}, nullptr, 0, dl::DATA_TYPE_FLOAT);
    memcpy(input_tensor->data, input_scaled, sizeof(input_scaled));
    model_input->assign(input_tensor);

    model->run();

    float* raw_output_data = (float*)model_output->data;
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        float logit = raw_output_data[i];
        float prob = sigmoid(logit);
        int class_label = (prob >= 0.5f) ? 1 : 0;
        const char* risk = (class_label == 1) ? "Low Risk" : "High Risk";

        printf("\n===== Kết quả AI =====\n");
        printf("HR=%.1f | Temp=%.2f | SpO2=%.2f | Risk=%s\n",
               input_data[i][0], input_data[i][1], input_data[i][2], risk);
    }

    delete input_tensor;
    delete model;
}

// ================= Draw UI =================
void drawInterface() {
    if(xSemaphoreTake(tftMutex, portMAX_DELAY)) {
        tft.fillScreen(TFT_BLACK);
        tft.fillRoundRect(0,0,240,40,8,TFT_BLUE);
        tft.setTextColor(TFT_WHITE, TFT_BLUE);
        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(2);
        tft.drawString("HEALTH MONITOR",120,20);

        tft.setTextSize(2);
        tft.drawRoundRect(5,45,230,70,5,TFT_WHITE);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setCursor(10,50); tft.print("TIME");

        tft.drawRoundRect(5,120,110,60,5,TFT_WHITE); tft.setCursor(10,125); tft.print("HEART");
        tft.drawRoundRect(125,120,110,60,5,TFT_WHITE); tft.setCursor(130,125); tft.print("TEMP");
        tft.drawRoundRect(5,190,110,60,5,TFT_WHITE); tft.setCursor(10,195); tft.print("SPO2");
        tft.drawRoundRect(125,190,110,60,5,TFT_WHITE); tft.setCursor(130,195); tft.print("STEPS");

        xSemaphoreGive(tftMutex);
    }
}

// ================= RTC Display Task =================
void task_rtc_display(void *pvParameters) {
    while (true) {
        update_time_display();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void update_time_display() {
    Time t = rtc.time();
    const String day = dayAsString(t.day);

    char timeBuf[40];
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", t.hr, t.min, t.sec);

    if(xSemaphoreTake(tftMutex, portMAX_DELAY)) {
        tft.setTextDatum(TL_DATUM); tft.setTextSize(2);
        tft.fillRect(10,65,220,25,TFT_BLACK);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.drawString(timeBuf,15,70);
        xSemaphoreGive(tftMutex);
    }
}

// ================= Step Counter Task =================
void task_step_counter(void *pvParameters) {
    static float lastAccelMag = 0.0f;

    while (true) {
        sensors_event_t a, g, temp_event;
        mpu.getEvent(&a, &g, &temp_event);

        float accelMag = sqrtf(a.acceleration.x*a.acceleration.x +
                               a.acceleration.y*a.acceleration.y +
                               a.acceleration.z*a.acceleration.z);
        const float THRESH = 2.0f;
        if ((accelMag - lastAccelMag) > THRESH) stepCount++;
        lastAccelMag = accelMag;

        if(xSemaphoreTake(tftMutex, portMAX_DELAY)) {
            tft.setTextColor(TFT_CYAN, TFT_BLACK);
            char stepBuf[20]; sprintf(stepBuf,"%d", stepCount);
            tft.fillRect(125,210,100,25,TFT_BLACK);
            tft.drawString(stepBuf,135,215);
            xSemaphoreGive(tftMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// ================= Temperature Task =================
void task_temp_display(void *pvParameters) {
    while (true) {
        float temp = mlx.readObjectTempC();

        if (xSemaphoreTake(tftMutex, portMAX_DELAY)) {
            if (temp > 37.8) tft.setTextColor(TFT_RED, TFT_BLACK);
            else if (temp > 37.0) tft.setTextColor(TFT_YELLOW, TFT_BLACK);
            else tft.setTextColor(TFT_WHITE, TFT_BLACK);

            char tempBuf[20];
            sprintf(tempBuf, "%.1f C", temp);
            tft.fillRect(130, 145, 110, 40, TFT_BLACK);
            tft.setTextDatum(TL_DATUM);
            tft.setTextSize(2);
            tft.drawString(tempBuf, 135, 150);

            xSemaphoreGive(tftMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// ================= Button ISR =================
void IRAM_ATTR buttonISR() {
    deepSleepRequested = true;
}

// ================= Deep Sleep Task =================
void task_deep_sleep(void *pvParameters) {
    while (true) {
        if (deepSleepRequested) {
            if (xSemaphoreTake(tftMutex, pdMS_TO_TICKS(500))) {
                tft.fillScreen(TFT_BLACK);
                xSemaphoreGive(tftMutex);
            }
            printf("Entering Deep Sleep...\n");
            vTaskDelay(pdMS_TO_TICKS(100));

            // Wake-up từ GPIO1 (nhấn LOW)
            esp_sleep_enable_ext0_wakeup(GPIO_NUM_1, 0);

            esp_deep_sleep_start();
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// ================= Setup Sensors & RTC =================
void setup_sensors() {
    Wire.begin(9, 8, 400000);

    if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) { while (1) vTaskDelay(pdMS_TO_TICKS(1000)); }
    particleSensor.setup(80, 4, 2, 100, 411, 4096);

    if (!mlx.begin()) { while (1) vTaskDelay(pdMS_TO_TICKS(1000)); }

    if (!mpu.begin()) { while (1) vTaskDelay(pdMS_TO_TICKS(1000)); } 
    else {
        mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
        mpu.setGyroRange(MPU6050_RANGE_500_DEG);
        mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    }

    rtc.halt(false);
    rtc.writeProtect(false);
    Time t(2025, 10, 19, 8, 30, 50, Time::kSunday);
    rtc.time(t);

    tft.init();
    tft.setRotation(0);

    // Tạo mutex TFT
    tftMutex = xSemaphoreCreateMutex();
    if(!tftMutex) while(1);

    drawInterface();

    // Setup button
    pinMode(BUTTON_DEEP_SLEEP, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_DEEP_SLEEP), buttonISR, FALLING);
}

// ================= MAIN =================
extern "C" void app_main(void) {
    // Kiểm tra wake-up
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause == ESP_SLEEP_WAKEUP_EXT0) {
        printf("Woke up from GPIO1!\n");
    }

    setup_sensors();

    // Tạo tasks
    xTaskCreate(task_rtc_display, "RTC Display Task", 4096, NULL, 1, NULL);
    xTaskCreate(task_step_counter, "Step Counter Task", 4096, NULL, 1, NULL);
    xTaskCreate(task_temp_display, "Temp Display Task", 4096, NULL, 1, NULL);
    xTaskCreate(task_deep_sleep, "Deep Sleep Task", 2048, NULL, 1, NULL);

    while (true) {
        float input_data[NUM_SAMPLES][NUM_FEATURES];
        if (get_sensor_input_data(input_data)) {
            run_hvsd_model(input_data);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
*/

/* Part 9: Full AIoT Code + Deep Sleep + Firebase */

/* Final 1 */ 
/*
#include <iostream>
#include "dl_model_base.hpp"
#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include "Adafruit_MLX90614.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <TFT_eSPI.h>
#include "DS1302.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "time.h" // Thư viện cho timestamp
#include "esp_sntp.h" // Thư viện cho đồng bộ thời gian NTP

// --- FIREBASE ---
#define JSON_USE_EXCEPTION 0
#include "jsoncpp/value.h"
#include "jsoncpp/json.h"
#include "esp_firebase/app.h"
#include "esp_firebase/rtdb.h"
#include "wifi_utils.h"
#include "firebase_config.h"

using namespace ESPFirebase;
#define USER_ID "8ikZgX3e6gObPOdCHWlTAFMpd5j2"
// ====== Config ======
#define NUM_SAMPLES 1
#define NUM_FEATURES 7
#define DS1302_RST   2
#define DS1302_IO    3
#define DS1302_CLK   4
#define BUTTON_DEEP_SLEEP 1

// ====== Global Data ======
volatile int g_heartRate = 0;
volatile int g_spo2 = 0;
volatile float g_temperature = 0.0f;
volatile int g_stepCount = 0;
volatile const char* g_aiRiskResult = "Calculating...";
volatile bool deepSleepRequested = false;

// ====== AI Model & Display ======
extern const uint8_t model_espdl[] asm("_binary_model_espdl_start");
dl::Model *model;
TFT_eSPI tft = TFT_eSPI();
SemaphoreHandle_t tftMutex;

// ====== Sensors ======
MAX30105 particleSensor;
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
Adafruit_MPU6050 mpu;
DS1302 rtc(DS1302_RST, DS1302_IO, DS1302_CLK);

// ====== Normalization ======
const float means[NUM_FEATURES] = {79.5337f, 36.7483f, 97.5043f, 53.4462f, 0.4994f, 74.9964f, 1.7500f};
const float stds[NUM_FEATURES]  = {11.5528f, 0.4332f, 1.4425f, 20.7867f, 0.4999f, 14.4714f, 0.1445f};

float sigmoid(float x) { return 1.0f / (1.0f + expf(-x)); }

// ================= ISR =================
void IRAM_ATTR buttonISR() { deepSleepRequested = true; }

// ================= Draw UI =================
void drawInterface() {
    if(xSemaphoreTake(tftMutex, portMAX_DELAY)) {
        tft.fillScreen(TFT_BLACK);
        tft.fillRoundRect(0,0,240,40,8,TFT_BLUE);
        tft.setTextColor(TFT_WHITE, TFT_BLUE);
        tft.setTextDatum(MC_DATUM); tft.setTextSize(2);
        tft.drawString("HEALTH MONITOR",120,20);

        tft.setTextSize(2);
        tft.drawRoundRect(5,45,230,50,5,TFT_WHITE); tft.setCursor(10,50); tft.print("TIME");
        tft.drawRoundRect(5,100,110,60,5,TFT_WHITE); tft.setCursor(10,105); tft.print("HEART");
        tft.drawRoundRect(125,100,110,60,5,TFT_WHITE); tft.setCursor(130,105); tft.print("TEMP");
        tft.drawRoundRect(5,165,110,60,5,TFT_WHITE); tft.setCursor(10,170); tft.print("SPO2");
        tft.drawRoundRect(125,165,110,60,5,TFT_WHITE); tft.setCursor(130,170); tft.print("STEPS");
        tft.drawRoundRect(5,230,230,40,5,TFT_WHITE); tft.setCursor(10,235); tft.print("RISK STATUS");
        xSemaphoreGive(tftMutex);
    }
}

// ================= Display Updater =================
void task_display_updater(void *pvParameters) {
    char buf[40];
    while (true) {
        if (xSemaphoreTake(tftMutex, portMAX_DELAY)) {
            Time t = rtc.time();
            snprintf(buf, sizeof(buf), "%02d:%02d:%02d", t.hr, t.min, t.sec);
            tft.setTextDatum(TL_DATUM); tft.setTextSize(2);
            tft.fillRect(10,65,220,25,TFT_BLACK);
            tft.setTextColor(TFT_CYAN, TFT_BLACK);
            tft.drawString(buf, 15, 70);

            snprintf(buf, sizeof(buf), "%d BPM", g_heartRate);
            tft.setTextColor(TFT_GREEN, TFT_BLACK);
            tft.fillRect(10,125,100,25,TFT_BLACK);
            tft.drawString(buf, 15,130);

            if (g_temperature > 37.8) tft.setTextColor(TFT_RED, TFT_BLACK);
            else if (g_temperature > 37.0) tft.setTextColor(TFT_YELLOW, TFT_BLACK);
            else tft.setTextColor(TFT_WHITE, TFT_BLACK);
            snprintf(buf, sizeof(buf), "%.1f C", g_temperature);
            tft.fillRect(130,125,100,25,TFT_BLACK);
            tft.drawString(buf,135,130);

            snprintf(buf, sizeof(buf), "%d %%", g_spo2);
            tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
            tft.fillRect(10,190,100,25,TFT_BLACK);
            tft.drawString(buf,15,195);

            snprintf(buf, sizeof(buf), "%d", g_stepCount);
            tft.setTextColor(TFT_CYAN, TFT_BLACK);
            tft.fillRect(125,190,100,25,TFT_BLACK);
            tft.drawString(buf,135,195);

            tft.setTextDatum(MC_DATUM); tft.setTextSize(2);
            tft.fillRect(10,255,220,25,TFT_BLACK);
            if (strcmp((const char*)g_aiRiskResult, "High Risk") == 0) tft.setTextColor(TFT_RED, TFT_BLACK);
            else tft.setTextColor(TFT_GREEN, TFT_BLACK);
            tft.drawString((const char*)g_aiRiskResult,120,260);

            xSemaphoreGive(tftMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// ================= Sensors =================
void task_step_counter(void *pvParameters) {
    static const char *TAG = "STEP_COUNTER";
    static float accelFiltered = 0.0f;
    static bool stepDetected = false;
    const float THRESHOLD_HIGH = 13.5f;  // Ngưỡng trên phát hiện bước
    const float THRESHOLD_LOW  = 12.45f;  // Ngưỡng dưới để reset
    const float ALPHA = 0.1f;           // Hệ số lọc nhiễu (0.05–0.3 tùy độ nhạy)

    while (true) {
        sensors_event_t a, g, temp_event;
        mpu.getEvent(&a, &g, &temp_event);

        // Tính độ lớn vector gia tốc (magnitude)
        float accelMag = sqrt(a.acceleration.x * a.acceleration.x +
                               a.acceleration.y * a.acceleration.y +
                               a.acceleration.z * a.acceleration.z);
        // printf("Magnitude: %f\n", accelMag);
        // Lọc nhiễu (low-pass filter)
        accelFiltered = ALPHA * accelMag + (1.0f - ALPHA) * accelFiltered;

        // Phát hiện bước khi vượt qua ngưỡng cao
        if (!stepDetected && accelFiltered > THRESHOLD_HIGH) {
            g_stepCount++;       // Giữ nguyên biến này
            stepDetected = true; // Đánh dấu đã phát hiện 1 bước
        }

        // Reset cờ sau khi về dưới ngưỡng thấp
        if (stepDetected && accelFiltered < THRESHOLD_LOW) {
            stepDetected = false;
        }

        // ESP_LOGI(TAG,
                // "Raw=%.3f | Filtered=%.3f | StepDetected=%d | Steps=%d | AccelFitered=%.3f | THRESHOLD_HIGH=%.2f | THRESHOLD_LOW=%.2f",
                // accelMag, accelFiltered, stepDetected, g_stepCount, accelFiltered, THRESHOLD_HIGH, THRESHOLD_LOW);

        vTaskDelay(pdMS_TO_TICKS(100)); // Lấy mẫu mỗi 0.1 giây
    }
}



void task_temp_reader(void *pvParameters) {
    while (true) {
        g_temperature = mlx.readObjectTempC();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ================= AI =================
const char* run_hvsd_model(float input_data[NUM_SAMPLES][NUM_FEATURES]) {
    dl::TensorBase *in = model->get_inputs().begin()->second;
    dl::TensorBase *out = model->get_outputs().begin()->second;
    float input_scaled[NUM_SAMPLES][NUM_FEATURES];
    for (int j = 0; j < NUM_FEATURES; ++j)
        input_scaled[0][j] = (input_data[0][j] - means[j]) / stds[j];
    memcpy(in->data, input_scaled, sizeof(input_scaled));
    model->run();
    float prob = sigmoid(((float*)out->data)[0]);
    printf("\nAI: HR=%.1f Temp=%.2f SpO2=%.1f Logit=%.2f Prob=%.2f\n",
           input_data[0][0],input_data[0][1],input_data[0][2],((float*)out->data)[0],prob);
    return (prob >= 0.5f) ? "High Risk" : "Low Risk";
}

// ================= Main Processing =================
void task_main_processing(void *pvParameters) {
    float input_data[NUM_SAMPLES][NUM_FEATURES];
    static uint32_t irBuffer[100], redBuffer[100];
    const int32_t bufferLength = 100;
    while (true) {
        for (int i=0;i<bufferLength;i++) {
            while(!particleSensor.check()) vTaskDelay(pdMS_TO_TICKS(1));
            redBuffer[i]=particleSensor.getRed();
            irBuffer[i]=particleSensor.getIR();
        }
        int32_t spo2_val=0, hr_val=0;
        int8_t validSPO2=0, validHeartRate=0;
        maxim_heart_rate_and_oxygen_saturation(irBuffer,bufferLength,redBuffer,&spo2_val,&validSPO2,&hr_val,&validHeartRate);
        if (validSPO2 && validHeartRate && spo2_val>=93 && hr_val>=50 && hr_val<=150) {
            g_heartRate=hr_val; g_spo2=spo2_val;
            input_data[0][0]=g_heartRate; input_data[0][1]=g_temperature; input_data[0][2]=g_spo2;
            input_data[0][3]=22.0f; input_data[0][4]=0.0f; input_data[0][5]=70.0f; input_data[0][6]=1.7f;
            g_aiRiskResult=run_hvsd_model(input_data);
        } else {
            g_heartRate=0; g_spo2=0; g_aiRiskResult="Wait for Signal";
            printf("Invalid HR=%ld SpO2=%ld\n",hr_val,spo2_val);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ================= Deep Sleep =================
void task_deep_sleep(void *pvParameters) {
    while (true) {
        if (deepSleepRequested) {
            if (xSemaphoreTake(tftMutex,pdMS_TO_TICKS(500))) { tft.fillScreen(TFT_BLACK); xSemaphoreGive(tftMutex); }
            printf("Entering Deep Sleep...\n");
            vTaskDelay(pdMS_TO_TICKS(100));
            esp_sleep_enable_ext0_wakeup(GPIO_NUM_1,0);
            esp_deep_sleep_start();
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// ======================= FIREBASE (Fixed) =======================
// ======================= TASK GỬI DỮ LIỆU LÊN FIREBASE (ĐÃ FIX) =======================
void task_send_to_firebase(void *pvParameters) {
    // 1. Kết nối WiFi
    wifiInit(SSID, PASSWORD);

    // 2. Đăng nhập Firebase
    vTaskDelay(pdMS_TO_TICKS(10000));
    user_account_t account = {USER_EMAIL, USER_PASSWORD};
    FirebaseApp app = FirebaseApp(API_KEY);
    esp_err_t login_status = app.loginUserAccount(account);
    if (login_status != ESP_OK) {
        ESP_LOGE("FIREBASE", "Login failed (err=0x%x)", login_status);
        vTaskDelete(NULL);
    }
    ESP_LOGI("FIREBASE", "Firebase login successful!");
    
    // 3. Khởi tạo RTDB
    RTDB db = RTDB(&app, DATABASE_URL);
    // 4. Gửi dữ liệu vòng lặp
    while (true) {
        
        Json::Value health_data;

        // --- LẤY SNAPSHOT AN TOÀN CHO CÁC GIÁ TRỊ CHUNG ---
        int bpm_snapshot = g_heartRate;
        int spo2_snapshot = g_spo2;
        float temp_snapshot = g_temperature;
        int steps_snapshot = g_stepCount;

        // Copy an toàn chuỗi risk (snapshot)
        std::string risk_snapshot;
        if (g_aiRiskResult != nullptr) {
            risk_snapshot = std::string((const char*)g_aiRiskResult);
        } else {
            risk_snapshot = "Unknown";
        }

        // Tạo riskLevel giúp xử lý phía Firebase / UI dễ hơn
        int riskLevel = -1; // -1 = invalid/unknown, 0 = Low, 1 = High
        if (risk_snapshot == "High Risk") riskLevel = 1;
        else if (risk_snapshot == "Low Risk") riskLevel = 0;
        else if (risk_snapshot == "Invalid Signal") riskLevel = -1;

        // --- Điền Json ---
        health_data["bpm"] = bpm_snapshot;
        health_data["spo2"] = spo2_snapshot;
        health_data["temperature"] = temp_snapshot;
        health_data["steps"] = steps_snapshot;
        health_data["riskStatus"] = risk_snapshot;   // chuỗi rõ ràng
        health_data["riskLevel"] = riskLevel;       // numeric flag
       
        // Timestamp snapshot (nếu cần chính xác)
        Time t = rtc.time();
        char timestamp[32];
        snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d",
                 t.yr, t.mon, t.date, t.hr, t.min, t.sec);
        health_data["timestamp"] = timestamp;

        // Ghi lên path users/<USER_ID>/realtime (ghi đè)
        std::string path = std::string("users/") + USER_ID + "/realtime";
        esp_err_t result = db.putData(path.c_str(), health_data);
        if (result == ESP_OK) {
            Json::FastWriter writer;
            std::string data_str = writer.write(health_data);
            ESP_LOGI("FIREBASE", "Data uploaded: %s", data_str.c_str());
        } else {
            ESP_LOGE("FIREBASE", "Failed to upload data (err=0x%x)", result);
        }

        vTaskDelay(pdMS_TO_TICKS(2000)); // 10 giây
    }
}



// ================= Setup =================
void setup_hardware() {
    Wire.begin(9,8,400000);
    if (!particleSensor.begin(Wire,I2C_SPEED_STANDARD)) while(1);
    particleSensor.setup(40,4,2,200,411,4096);
    if (!mlx.begin()) while(1);
    if (!mpu.begin()) while(1);
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    rtc.halt(false); rtc.writeProtect(false);
    tft.init(); tft.setRotation(0);
    tftMutex=xSemaphoreCreateMutex();
    model=new dl::Model((const char*)model_espdl,fbs::MODEL_LOCATION_IN_FLASH_RODATA);
    ESP_ERROR_CHECK(model->test());
    pinMode(BUTTON_DEEP_SLEEP,INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_DEEP_SLEEP),buttonISR,FALLING);
}

// ================= MAIN =================
extern "C" void app_main(void) {
    esp_log_level_set("gpio", ESP_LOG_WARN);
    setup_hardware();
    drawInterface();
    xTaskCreate(task_display_updater,"Display Task",4096,NULL,2,NULL);
    xTaskCreate(task_step_counter,"Step Counter",4096,NULL,1,NULL);
    xTaskCreate(task_temp_reader,"Temp Reader",2048,NULL,1,NULL);
    xTaskCreate(task_main_processing,"Main Processing",4096,NULL,1,NULL);
    xTaskCreate(task_deep_sleep,"Deep Sleep",2048,NULL,3,NULL);
    xTaskCreate(task_send_to_firebase,"Firebase Task",8192,NULL,2,NULL);
}
*/
/* Update giữ nguyên giá trị HR và SpO2 hợp lệ (last valid) cho đến khi có giá trị hợp lệ mới */

#include <iostream>
#include "dl_model_base.hpp"
#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include "Adafruit_MLX90614.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <TFT_eSPI.h>
#include "DS1302.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "time.h" // Thư viện cho timestamp
#include "esp_sntp.h" // Thư viện cho đồng bộ thời gian NTP

// --- FIREBASE ---
#define JSON_USE_EXCEPTION 0
#include "jsoncpp/value.h"
#include "jsoncpp/json.h"
#include "esp_firebase/app.h"
#include "esp_firebase/rtdb.h"
#include "wifi_utils.h"
#include "firebase_config.h"

using namespace ESPFirebase;
#define USER_ID "8ikZgX3e6gObPOdCHWlTAFMpd5j2"
// ====== Config ======
#define NUM_SAMPLES 1
#define NUM_FEATURES 7
#define DS1302_RST   2
#define DS1302_IO    3
#define DS1302_CLK   4
#define BUTTON_DEEP_SLEEP 1

// ====== Global Data ======
volatile int g_heartRate = 0;
volatile int g_spo2 = 0;
volatile float g_temperature = 0.0f;
volatile int g_stepCount = 0;
const char* g_aiRiskResult = "Calculating...";
volatile bool deepSleepRequested = false;

// ====== Last valid snapshot (keeps last good HR/SpO2) ======
volatile int last_valid_heartRate = 0;
volatile int last_valid_spo2 = 0;
volatile bool has_valid_measurement = false; // false until first valid measurement

// Mutex for protecting shared health data (heart/spo2/risk)
SemaphoreHandle_t dataMutex = NULL;

// ====== AI Model & Display ======
extern const uint8_t model_espdl[] asm("_binary_model_espdl_start");
dl::Model *model;
TFT_eSPI tft = TFT_eSPI();
SemaphoreHandle_t tftMutex;

// ====== Sensors ======
MAX30105 particleSensor;
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
Adafruit_MPU6050 mpu;
DS1302 rtc(DS1302_RST, DS1302_IO, DS1302_CLK);

// ====== Normalization ======
const float means[NUM_FEATURES] = {79.5337f, 36.7483f, 97.5043f, 53.4462f, 0.4994f, 74.9964f, 1.7500f};
const float stds[NUM_FEATURES]  = {11.5528f, 0.4332f, 1.4425f, 20.7867f, 0.4999f, 14.4714f, 0.1445f};

float sigmoid(float x) { return 1.0f / (1.0f + expf(-x)); }

// ================= ISR =================
void IRAM_ATTR buttonISR() { deepSleepRequested = true; }

// ================= Draw UI =================
void drawInterface() {
    if(xSemaphoreTake(tftMutex, portMAX_DELAY)) {
        tft.fillScreen(TFT_BLACK);
        tft.fillRoundRect(0,0,240,40,8,TFT_BLUE);
        tft.setTextColor(TFT_WHITE, TFT_BLUE);
        tft.setTextDatum(MC_DATUM); tft.setTextSize(2);
        tft.drawString("HEALTH MONITOR",120,20);

        tft.setTextSize(2);
        tft.drawRoundRect(5,45,230,50,5,TFT_WHITE); tft.setCursor(10,50); tft.print("TIME");
        tft.drawRoundRect(5,100,110,60,5,TFT_WHITE); tft.setCursor(10,105); tft.print("HEART");
        tft.drawRoundRect(125,100,110,60,5,TFT_WHITE); tft.setCursor(130,105); tft.print("TEMP");
        tft.drawRoundRect(5,165,110,60,5,TFT_WHITE); tft.setCursor(10,170); tft.print("SPO2");
        tft.drawRoundRect(125,165,110,60,5,TFT_WHITE); tft.setCursor(130,170); tft.print("STEPS");
        tft.drawRoundRect(5,230,230,40,5,TFT_WHITE); tft.setCursor(10,235); tft.print("RISK STATUS");
        xSemaphoreGive(tftMutex);
    }
}

// ================= Display Updater =================
void task_display_updater(void *pvParameters) {
    char buf[40];
    while (true) {
        if (xSemaphoreTake(tftMutex, portMAX_DELAY)) {
            Time t = rtc.time();
            snprintf(buf, sizeof(buf), "%02d:%02d:%02d", t.hr, t.min, t.sec);
            tft.setTextDatum(TL_DATUM); tft.setTextSize(2);
            tft.fillRect(10,65,220,25,TFT_BLACK);
            tft.setTextColor(TFT_CYAN, TFT_BLACK);
            tft.drawString(buf, 15, 70);

            // Safely read the shared values
            int display_hr = 0;
            int display_spo2 = 0;
            const char* display_risk = NULL;
            int display_steps = 0;
            float display_temp = 0.0f;

            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(50))) {
                if (has_valid_measurement) {
                    display_hr = last_valid_heartRate;
                    display_spo2 = last_valid_spo2;
                } else {
                    display_hr = 0;
                    display_spo2 = 0;
                }
                display_risk = g_aiRiskResult;
                display_steps = g_stepCount;
                display_temp = g_temperature;
                xSemaphoreGive(dataMutex);
            }

            // HEART
            if (display_hr > 0) {
                snprintf(buf, sizeof(buf), "%d BPM", display_hr);
                tft.setTextColor(TFT_GREEN, TFT_BLACK);
            } else {
                snprintf(buf, sizeof(buf), "Wait");
                tft.setTextColor(TFT_YELLOW, TFT_BLACK);
            }
            tft.fillRect(10,125,100,25,TFT_BLACK);
            tft.drawString(buf, 15,130);

            // TEMP
            if (display_temp > 37.8) tft.setTextColor(TFT_RED, TFT_BLACK);
            else if (display_temp > 37.0) tft.setTextColor(TFT_YELLOW, TFT_BLACK);
            else tft.setTextColor(TFT_WHITE, TFT_BLACK);
            snprintf(buf, sizeof(buf), "%.1f C", display_temp);
            tft.fillRect(130,125,100,25,TFT_BLACK);
            tft.drawString(buf,135,130);

            // SPO2
            if (display_spo2 > 0) {
                snprintf(buf, sizeof(buf), "%d %%", display_spo2);
                tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
            } else {
                snprintf(buf, sizeof(buf), "-- %%");
                tft.setTextColor(TFT_YELLOW, TFT_BLACK);
            }
            tft.fillRect(10,190,100,25,TFT_BLACK);
            tft.drawString(buf,15,195);

            // STEPS
            snprintf(buf, sizeof(buf), "%d", display_steps);
            tft.setTextColor(TFT_CYAN, TFT_BLACK);
            tft.fillRect(125,190,100,25,TFT_BLACK);
            tft.drawString(buf,135,195);

            // RISK
            tft.setTextDatum(MC_DATUM); tft.setTextSize(2);
            tft.fillRect(10,255,220,25,TFT_BLACK);
            if (display_risk && strcmp(display_risk, "High Risk") == 0) tft.setTextColor(TFT_RED, TFT_BLACK);
            else tft.setTextColor(TFT_GREEN, TFT_BLACK);
            tft.drawString(display_risk ? display_risk : "Calculating...",120,260);

            xSemaphoreGive(tftMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// ================= Sensors =================
void task_step_counter(void *pvParameters) {
    static const char *TAG = "STEP_COUNTER";
    static float accelFiltered = 0.0f;
    static bool stepDetected = false;
    const float THRESHOLD_HIGH = 13.5f;  // Ngưỡng trên phát hiện bước
    const float THRESHOLD_LOW  = 12.45f;  // Ngưỡng dưới để reset
    const float ALPHA = 0.1f;           // Hệ số lọc nhiễu (0.05–0.3 tùy độ nhạy)

    while (true) {
        sensors_event_t a, g, temp_event;
        mpu.getEvent(&a, &g, &temp_event);

        // Tính độ lớn vector gia tốc (magnitude)
        float accelMag = sqrt(a.acceleration.x * a.acceleration.x +
                               a.acceleration.y * a.acceleration.y +
                               a.acceleration.z * a.acceleration.z);

        // Lọc nhiễu (low-pass filter)
        accelFiltered = ALPHA * accelMag + (1.0f - ALPHA) * accelFiltered;

        // Phát hiện bước khi vượt qua ngưỡng cao
        if (!stepDetected && accelFiltered > THRESHOLD_HIGH) {
            g_stepCount++;
            stepDetected = true;
        }

        // Reset cờ sau khi về dưới ngưỡng thấp
        if (stepDetected && accelFiltered < THRESHOLD_LOW) {
            stepDetected = false;
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // Lấy mẫu mỗi 0.1 giây
    }
}

void task_temp_reader(void *pvParameters) {
    while (true) {
        g_temperature = mlx.readObjectTempC();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ================= AI =================
const char* run_hvsd_model(float input_data[NUM_SAMPLES][NUM_FEATURES]) {
    dl::TensorBase *in = model->get_inputs().begin()->second;
    dl::TensorBase *out = model->get_outputs().begin()->second;
    float input_scaled[NUM_SAMPLES][NUM_FEATURES];
    for (int j = 0; j < NUM_FEATURES; ++j)
        input_scaled[0][j] = (input_data[0][j] - means[j]) / stds[j];
    memcpy(in->data, input_scaled, sizeof(input_scaled));
    model->run();
    float prob = sigmoid(((float*)out->data)[0]);
    printf("\nAI: HR=%.1f Temp=%.2f SpO2=%.1f Logit=%.2f Prob=%.2f\n",
           input_data[0][0],input_data[0][1],input_data[0][2],((float*)out->data)[0],prob);
    return (prob >= 0.5f) ? "High Risk" : "Low Risk";
}

// ================= Main Processing =================
// Thay đổi chính: không ghi đè last_valid nếu phép đo không hợp lệ.
// Chỉ cập nhật last_valid khi validSPO2 && validHeartRate && trong ngưỡng.
void task_main_processing(void *pvParameters) {
    float input_data[NUM_SAMPLES][NUM_FEATURES];
    static uint32_t irBuffer[100], redBuffer[100];
    const int32_t bufferLength = 100;
    while (true) {
        for (int i=0;i<bufferLength;i++) {
            while(!particleSensor.check()) vTaskDelay(pdMS_TO_TICKS(1));
            redBuffer[i]=particleSensor.getRed();
            irBuffer[i]=particleSensor.getIR();
        }
        int32_t spo2_val=0, hr_val=0;
        int8_t validSPO2=0, validHeartRate=0;
        maxim_heart_rate_and_oxygen_saturation(irBuffer,bufferLength,redBuffer,&spo2_val,&validSPO2,&hr_val,&validHeartRate);

        bool this_measurement_valid = (validSPO2 && validHeartRate && spo2_val>=93 && hr_val>=50 && hr_val<=150);

        if (this_measurement_valid) {
            // Cập nhật last_valid an toàn bằng mutex
            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(50))) {
                last_valid_heartRate = (int)hr_val;
                last_valid_spo2 = (int)spo2_val;
                has_valid_measurement = true;

                // Cập nhật các biến hiển thị/chung
                g_heartRate = last_valid_heartRate;
                g_spo2 = last_valid_spo2;

                // Chuẩn bị input cho AI: dùng last valid + các feature tĩnh/hiện tại
                input_data[0][0] = (float) last_valid_heartRate;
                input_data[0][1] = g_temperature; // current temperature reading
                input_data[0][2] = (float) last_valid_spo2;
                input_data[0][3] = 22.0f; input_data[0][4] = 0.0f; input_data[0][5] = 70.0f; input_data[0][6] = 1.7f;

                // Chạy model và cập nhật kết quả risk (giữ nguyên nếu model trả về same pointer literals)
                g_aiRiskResult = run_hvsd_model(input_data);

                xSemaphoreGive(dataMutex);
            }
        } else {
            // Nếu không valid thì KHÔNG ghi đè last_valid_xxx.
            // Nếu chưa có measurement hợp lệ nào (has_valid_measurement == false), giữ g_heartRate/g_spo2 = 0 và risk là "Wait for Signal"
            if (!has_valid_measurement) {
                if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(50))) {
                    g_heartRate = 0;
                    g_spo2 = 0;
                    g_aiRiskResult = "Wait for Signal";
                    xSemaphoreGive(dataMutex);
                }
            } else {
                // Đã có last valid trước đó -> giữ nguyên last_valid để hiển thị & gửi, không chạy model
                // (không làm gì thêm ở đây)
            }
            printf("Invalid HR=%ld SpO2=%ld (keep last valid if exists)\n",hr_val,spo2_val);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ================= Deep Sleep =================
void task_deep_sleep(void *pvParameters) {
    while (true) {
        if (deepSleepRequested) {
            if (xSemaphoreTake(tftMutex,pdMS_TO_TICKS(500))) { tft.fillScreen(TFT_BLACK); xSemaphoreGive(tftMutex); }
            printf("Entering Deep Sleep...\n");
            vTaskDelay(pdMS_TO_TICKS(100));
            esp_sleep_enable_ext0_wakeup(GPIO_NUM_1,0);
            esp_deep_sleep_start();
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// ======================= FIREBASE =======================
// Task gửi dùng last_valid_* (nếu có), nếu chưa có thì gửi 0 / "Unknown" / risk message tương ứng.
void task_send_to_firebase(void *pvParameters) {
    // 1. Kết nối WiFi
    wifiInit(SSID, PASSWORD);

    // 2. Đăng nhập Firebase
    vTaskDelay(pdMS_TO_TICKS(10000));
    user_account_t account = {USER_EMAIL, USER_PASSWORD};
    FirebaseApp app = FirebaseApp(API_KEY);
    esp_err_t login_status = app.loginUserAccount(account);
    if (login_status != ESP_OK) {
        ESP_LOGE("FIREBASE", "Login failed (err=0x%x)", login_status);
        vTaskDelete(NULL);
    }
    ESP_LOGI("FIREBASE", "Firebase login successful!");
    
    // 3. Khởi tạo RTDB
    RTDB db = RTDB(&app, DATABASE_URL);
    // 4. Gửi dữ liệu vòng lặp
    while (true) {
        
        Json::Value health_data;

        // --- LẤY SNAPSHOT AN TOÀN CHO CÁC GIÁ TRỊ CHUNG ---
        int bpm_snapshot = 0;
        int spo2_snapshot = 0;
        float temp_snapshot = 0.0f;
        int steps_snapshot = 0;
        std::string risk_snapshot = "Unknown";

        // Lấy an toàn từ mutex: dùng last_valid nếu có, ngược lại giữ 0 / "Wait for Signal"
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100))) {
            if (has_valid_measurement) {
                bpm_snapshot = last_valid_heartRate;
                spo2_snapshot = last_valid_spo2;
            } else {
                bpm_snapshot = 0;
                spo2_snapshot = 0;
            }
            temp_snapshot = g_temperature;
            steps_snapshot = g_stepCount;
            if (g_aiRiskResult != nullptr) risk_snapshot = std::string((const char*)g_aiRiskResult);
            xSemaphoreGive(dataMutex);
        }

        // Tạo riskLevel giúp xử lý phía Firebase / UI dễ hơn
        int riskLevel = -1; // -1 = invalid/unknown, 0 = Low, 1 = High
        if (risk_snapshot == "High Risk") riskLevel = 1;
        else if (risk_snapshot == "Low Risk") riskLevel = 0;
        else if (risk_snapshot == "Invalid Signal" || risk_snapshot == "Wait for Signal") riskLevel = -1;

        // --- Điền Json ---
        health_data["bpm"] = bpm_snapshot;
        health_data["spo2"] = spo2_snapshot;
        health_data["temperature"] = temp_snapshot;
        health_data["steps"] = steps_snapshot;
        health_data["riskStatus"] = risk_snapshot;   // chuỗi rõ ràng
        health_data["riskLevel"] = riskLevel;       // numeric flag
       
        // Timestamp snapshot (nếu cần chính xác)
        Time t = rtc.time();
        char timestamp[32];
        snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d",
                 t.yr, t.mon, t.date, t.hr, t.min, t.sec);
        health_data["timestamp"] = timestamp;

        // Ghi lên path users/<USER_ID>/realtime (ghi đè)
        std::string path = std::string("users/") + USER_ID + "/realtime";
        esp_err_t result = db.putData(path.c_str(), health_data);
        if (result == ESP_OK) {
            Json::FastWriter writer;
            std::string data_str = writer.write(health_data);
            ESP_LOGI("FIREBASE", "Data uploaded: %s", data_str.c_str());
        } else {
            ESP_LOGE("FIREBASE", "Failed to upload data (err=0x%x)", result);
        }

        vTaskDelay(pdMS_TO_TICKS(2000)); // 2 giây (ghi chú: comment ghi 10s trước là nhầm; hiện đặt 2s như mẫu)
    }
}

// ================= Setup =================
void setup_hardware() {
    Wire.begin(9,8,400000);
    if (!particleSensor.begin(Wire,I2C_SPEED_STANDARD)) while(1);
    particleSensor.setup(40,4,2,200,411,4096);
    if (!mlx.begin()) while(1);
    if (!mpu.begin()) while(1);
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    rtc.halt(false); rtc.writeProtect(false);
    tft.init(); tft.setRotation(0);
    tftMutex = xSemaphoreCreateMutex();
    dataMutex = xSemaphoreCreateMutex();
    model = new dl::Model((const char*)model_espdl,fbs::MODEL_LOCATION_IN_FLASH_RODATA);
    ESP_ERROR_CHECK(model->test());
    pinMode(BUTTON_DEEP_SLEEP,INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_DEEP_SLEEP),buttonISR,FALLING);
}

// ================= MAIN =================
extern "C" void app_main(void) {
    esp_log_level_set("gpio", ESP_LOG_WARN);
    setup_hardware();
    drawInterface();
    xTaskCreate(task_display_updater,"Display Task",4096,NULL,2,NULL);
    xTaskCreate(task_step_counter,"Step Counter",4096,NULL,1,NULL);
    xTaskCreate(task_temp_reader,"Temp Reader",2048,NULL,1,NULL);
    xTaskCreate(task_main_processing,"Main Processing",8192,NULL,1,NULL);
    xTaskCreate(task_deep_sleep,"Deep Sleep",2048,NULL,3,NULL);
    xTaskCreate(task_send_to_firebase,"Firebase Task",8192,NULL,2,NULL);
}
