
add_compile_options(-fdiagnostics-color=always)

list(APPEND EXTRA_COMPONENT_DIRS 
                                "${IOT_SOLUTION_PATH}/components"
                                "${IOT_SOLUTION_PATH}/components/audio"
                                "${IOT_SOLUTION_PATH}/examples/common_components"
                                "${IOT_SOLUTION_PATH}/components/bus"
                                "${IOT_SOLUTION_PATH}/components/button"
                                "${IOT_SOLUTION_PATH}/components/display"
                                "${IOT_SOLUTION_PATH}/components/display/digital_tube"
                                "${IOT_SOLUTION_PATH}/components/expander/io_expander"
                                "${IOT_SOLUTION_PATH}/components/gui"
                                "${IOT_SOLUTION_PATH}/components/led"
                                "${IOT_SOLUTION_PATH}/components/motor"
                                "${IOT_SOLUTION_PATH}/components/sensors"
                                "${IOT_SOLUTION_PATH}/components/sensors/gesture"
                                "${IOT_SOLUTION_PATH}/components/sensors/humiture"
                                "${IOT_SOLUTION_PATH}/components/sensors/imu"
                                "${IOT_SOLUTION_PATH}/components/sensors/light_sensor"
                                "${IOT_SOLUTION_PATH}/components/sensors/pressure"
                                "${IOT_SOLUTION_PATH}/components/storage"
                                "${IOT_SOLUTION_PATH}/components/storage/eeprom"
                                )

