# Find SDK Core files
file(GLOB_RECURSE SDK_SOURCES 
    "edge-impulse-sdk/classifier/*.cpp"
    "edge-impulse-sdk/dsp/*.cpp"
    "edge-impulse-sdk/dsp/*.c"
    "edge-impulse-sdk/third_party/*.c"
    "edge-impulse-sdk/third_party/*.cpp"
    "edge-impulse-sdk/third_party/*.cc"
    "edge-impulse-sdk/tensorflow/*.c"
    "edge-impulse-sdk/tensorflow/*.cpp"
    "edge-impulse-sdk/tensorflow/*.cc"
    "tflite-model/*.cpp"
    "model-parameters/*.cpp"
)

# Find Espressif Port files ONLY
file(GLOB_RECURSE PORT_SOURCES 
    "edge-impulse-sdk/porting/espressif/*.cpp" 
    "edge-impulse-sdk/porting/espressif/*.c"
    "edge-impulse-sdk/porting/espressif/*.S"
)

set(SOURCES ${SDK_SOURCES} ${PORT_SOURCES})

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS
        .
        edge-impulse-sdk
        model-parameters
        tflite-model
        edge-impulse-sdk/classifier
        edge-impulse-sdk/dsp
        edge-impulse-sdk/porting
        edge-impulse-sdk/third_party/flatbuffers/include
        edge-impulse-sdk/third_party/gemmlowp
        edge-impulse-sdk/third_party/ruy
        edge-impulse-sdk/porting/espressif/esp-dsp/modules/common/include
        edge-impulse-sdk/porting/espressif/esp-dsp/modules/fft/include
        edge-impulse-sdk/porting/espressif/ESP-NN
        edge-impulse-sdk/porting/espressif/ESP-NN/include
    REQUIRES
        esp_timer
        log
        driver
        freertos
)

# Compiler flags for Edge Impulse
target_compile_definitions(${COMPONENT_LIB} PUBLIC
    EI_CLASSIFIER_TFLITE_ENABLE_ESP_NN=1
    EI_CLASSIFIER_TFLITE_ENABLE_ESP_NN_S3=1
    EIDSP_USE_ESP_DSP=1
    CONFIG_DSP_MAX_FFT_SIZE=4096
)

# Suppress some warnings from the SDK
target_compile_options(${COMPONENT_LIB} PUBLIC 
    -mlongcalls
    -Wno-unused-variable 
    -Wno-unused-function 
    -Wno-missing-field-initializers
    -Wno-maybe-uninitialized
    -Wno-deprecated-declarations
    -Wno-format
)
