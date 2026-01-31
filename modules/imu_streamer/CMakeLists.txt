idf_component_register(SRCS "src/imu_streamer.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES "sensor_icm42607" "esp_timer" "log" "edge_impulse" "console" "esp_driver_usb_serial_jtag")
