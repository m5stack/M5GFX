set(COMPONENT_ADD_INCLUDEDIRS
    src
    )
file(GLOB SRCS
     src/*.cpp
     src/lgfx/Fonts/efont/*.c
     src/lgfx/Fonts/IPA/*.c
     src/lgfx/utility/*.c
     src/lgfx/v1/*.cpp
     src/lgfx/v1/misc/*.cpp
     src/lgfx/v1/panel/*.cpp
     src/lgfx/v1/platforms/esp32/*.cpp
     src/lgfx/v1/touch/*.cpp
     )

# Keep common ESP32 sources, then add only the active chip's implementation.
# This also prevents PlatformIO's flat object layout from colliding on same-named
# sources that live in different platform directories.
if(IDF_TARGET STREQUAL "esp32c3")
    file(GLOB PLATFORM_SRCS src/lgfx/v1/platforms/esp32c3/*.cpp)
elseif(IDF_TARGET STREQUAL "esp32s2")
    file(GLOB PLATFORM_SRCS src/lgfx/v1/platforms/esp32s2/*.cpp)
elseif(IDF_TARGET STREQUAL "esp32s3")
    file(GLOB PLATFORM_SRCS src/lgfx/v1/platforms/esp32s3/*.cpp)
elseif(IDF_TARGET STREQUAL "esp32p4")
    file(GLOB PLATFORM_SRCS src/lgfx/v1/platforms/esp32p4/*.cpp)
endif()
list(APPEND SRCS ${PLATFORM_SRCS})
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

### If you use arduino-esp32 components, please activate next comment line.
# list(APPEND COMPONENT_REQUIRES arduino-esp32)


message(STATUS "M5GFX use components = ${COMPONENT_REQUIRES}")

register_component()
