idf_component_register(SRCS "esp-dsp.cpp" "subsonic.cpp" "pregain.cpp" "equalizer.cpp" "limiter.cpp" "serial_commands.cpp" "wifi_manager.cpp" "mqtt_manager.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES driver nvs_flash esp_wifi esp_netif esp_event mqtt)
