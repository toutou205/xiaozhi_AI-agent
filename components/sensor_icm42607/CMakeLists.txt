idf_component_register(SRCS "src/sensor_icm42607.c"
                       INCLUDE_DIRS "include"
                       REQUIRES driver esp_timer log freertos) 
                       # Managed component icm42670 should be required.
