set(COMPONENT_ADD_INCLUDEDIRS
    src
    )
# The build set is fixed: src/lgfx/v1/lgfx_v1.cpp includes every implementation file (*.inl),
# the font tables and the C decoders are separate translation units on purpose.
set(SRCS
     src/M5GFX.cpp
     src/lgfx/v1/lgfx_v1.cpp
     src/lgfx/Fonts/efont/lgfx_efont_cn.c
     src/lgfx/Fonts/efont/lgfx_efont_ja.c
     src/lgfx/Fonts/efont/lgfx_efont_kr.c
     src/lgfx/Fonts/efont/lgfx_efont_tw.c
     src/lgfx/Fonts/IPA/lgfx_font_japan.c
     src/lgfx/utility/lgfx_miniz.c
     src/lgfx/utility/lgfx_pngle.c
     src/lgfx/utility/lgfx_qoi.c
     src/lgfx/utility/lgfx_qrcode.c
     src/lgfx/utility/lgfx_tjpgd.c
     )
set(COMPONENT_SRCS ${SRCS})

if (IDF_VERSION_MAJOR GREATER_EQUAL 6)
    set(COMPONENT_REQUIRES nvs_flash efuse esp_lcd driver esp_timer esp_mm esp_driver_ledc esp_driver_i2s hal)
elseif (IDF_VERSION_MAJOR GREATER_EQUAL 5)
    set(COMPONENT_REQUIRES nvs_flash efuse driver esp_timer esp_lcd)
    if("${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}" VERSION_GREATER_EQUAL "5.1")
        list(APPEND COMPONENT_REQUIRES esp_mm)
    endif()
elseif ((IDF_VERSION_MAJOR EQUAL 4) AND (IDF_VERSION_MINOR GREATER 3) OR IDF_VERSION_MAJOR GREATER 4)
    set(COMPONENT_REQUIRES nvs_flash efuse esp_lcd)
else()
    set(COMPONENT_REQUIRES nvs_flash efuse)
endif()



message(STATUS "M5GFX use components = ${COMPONENT_REQUIRES}")

register_component()

# Arduino as an ESP-IDF component: arduino-esp32 publishes -DARDUINO... as PUBLIC compile options, so
# they only reach components that link against it, while the public headers of this library change
# class layout with ARDUINO. Link the Arduino component publicly whenever it is part of the build so
# this library is compiled in the same mode as the application. Only components already selected
# for the build are considered; a build without Arduino is unaffected.
#   M5GFX_ARDUINO_COMPONENT=<name>  use a differently named Arduino component
#   M5GFX_ARDUINO_COMPONENT=OFF     disable the automatic dependency
set(_m5gfx_arduino_candidates arduino arduino-esp32 espressif__arduino-esp32)
set(_m5gfx_arduino_enabled ON)
if(DEFINED M5GFX_ARDUINO_COMPONENT AND NOT "${M5GFX_ARDUINO_COMPONENT}" STREQUAL "")
    if(M5GFX_ARDUINO_COMPONENT)
        set(_m5gfx_arduino_candidates ${M5GFX_ARDUINO_COMPONENT})
    else()
        set(_m5gfx_arduino_enabled OFF)
    endif()
endif()
if(_m5gfx_arduino_enabled)
    idf_build_get_property(_m5gfx_arduino_build_components BUILD_COMPONENTS)
    set(_m5gfx_arduino_hits)
    foreach(_m5gfx_arduino_name ${_m5gfx_arduino_candidates})
        if(_m5gfx_arduino_name IN_LIST _m5gfx_arduino_build_components)
            list(APPEND _m5gfx_arduino_hits ${_m5gfx_arduino_name})
        endif()
    endforeach()
    list(LENGTH _m5gfx_arduino_hits _m5gfx_arduino_count)
    if(_m5gfx_arduino_count GREATER 1)
        message(FATAL_ERROR "M5GFX: several Arduino components are in the build (${_m5gfx_arduino_hits}). Set M5GFX_ARDUINO_COMPONENT to the one to use.")
    elseif(_m5gfx_arduino_count EQUAL 1)
        idf_component_get_property(_m5gfx_arduino_lib ${_m5gfx_arduino_hits} COMPONENT_LIB)
        target_link_libraries(${COMPONENT_LIB} PUBLIC ${_m5gfx_arduino_lib})
    endif()
endif()
