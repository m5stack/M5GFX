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
     src/lgfx/v1/platforms/esp32c3/*.cpp
     src/lgfx/v1/platforms/esp32s3/*.cpp
     src/lgfx/v1/touch/*.cpp
     )
set(COMPONENT_SRCS ${SRCS})
set(COMPONENT_REQUIRES nvs_flash efuse)


### If you use arduino-esp32 components, please activate next comment line.
# list(APPEND COMPONENT_REQUIRES arduino-esp32)


message(STATUS "M5GFX use components = ${COMPONENT_REQUIRES}")

register_component()
