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
