idf_build_get_property(component_targets __COMPONENT_TARGETS)
if ("___idf_espressif__esp-dl" IN_LIST component_targets)
    idf_component_get_property(espdl_dir espressif__esp-dl COMPONENT_DIR)
elseif("___idf_esp-dl" IN_LIST component_targets)
    idf_component_get_property(espdl_dir esp-dl COMPONENT_DIR)
endif()
set(cmake_dir ${espdl_dir}/fbs_loader/cmake)
include(${cmake_dir}/utilities.cmake)
set(embed_files models/model.espdl)

idf_component_register(SRCS "app_main.cpp"
                       REQUIRES esp-dl arduino Adafruit_BusIO Adafruit-MLX90614-Library SparkFun_MAX3010x_Sensor_Library Adafruit_Sensor Adafruit-GFX-Library Adafruit_SSD1306 Adafruit_MPU6050 arduino-ds1302 TFT_eSPI nvs_flash json
                       INCLUDE_DIRS "." "../components"
                       )

target_add_aligned_binary_data(${COMPONENT_LIB} ${embed_files} BINARY)
add_compile_options(-Wno-error=overloaded-virtual)
