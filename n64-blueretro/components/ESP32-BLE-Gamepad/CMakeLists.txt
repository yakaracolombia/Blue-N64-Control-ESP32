idf_component_register(SRCS 
                    "BleConnectionStatus.cpp" "BleGamepad.cpp" 
                    "BleGamepadConfiguration.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES driver
                    esp_common esp_system
                    esp-nimble-cpp)

