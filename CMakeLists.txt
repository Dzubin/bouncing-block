# PicoCalc bouncing-block demo

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

include(pico_sdk_import.cmake)

project(bouncing-block C CXX ASM)

pico_sdk_init()

add_executable(bouncing-block
        main.c
        game.c
        game.h
        constants.h
        drivers/lcd.c
        drivers/lcd.h
        drivers/font-8x10.c
        drivers/font.h
        drivers/keyboard.c
        drivers/keyboard.h
        drivers/southbridge.c
        drivers/southbridge.h
        drivers/audio.c
        drivers/audio.h
        )

pico_set_program_name(bouncing-block "bouncing-block")
pico_set_program_version(bouncing-block "0.1")

# Generate the PIO header used by the audio driver.
pico_generate_pio_header(bouncing-block ${CMAKE_CURRENT_LIST_DIR}/drivers/audio.pio)

# No stdio needed for this demo.
pico_enable_stdio_uart(bouncing-block 0)
pico_enable_stdio_usb(bouncing-block 0)

target_compile_options(bouncing-block PRIVATE -Wall -Werror)

target_include_directories(bouncing-block PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(bouncing-block
        pico_stdlib
        pico_multicore
        pico_float
        hardware_spi
        hardware_watchdog   # watchdog_reboot(): leaving for the UF2 Loader
        hardware_gpio
        hardware_i2c
        hardware_pio
        hardware_clocks
        )

# The chip is part of every output file name (bouncing-block-RP2040.uf2 /
# bouncing-block-RP2350.uf2, and the matching .elf/.bin/.hex), so a downloaded file
# always says which board it was built for.
if(PICO_PLATFORM MATCHES "^rp2040")
    set(CHIP_NAME RP2040)
elseif(PICO_PLATFORM MATCHES "^rp2350")
    set(CHIP_NAME RP2350)
else()
    set(CHIP_NAME ${PICO_PLATFORM})
endif()
set_target_properties(bouncing-block PROPERTIES OUTPUT_NAME bouncing-block-${CHIP_NAME})

pico_add_extra_outputs(bouncing-block)

# Copy the finished .uf2 (same name) to the top-level project folder, so the
# RP2040 and RP2350 builds can sit side by side.
add_custom_command(TARGET bouncing-block POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE_DIR:bouncing-block>/$<TARGET_FILE_BASE_NAME:bouncing-block>.uf2
            ${CMAKE_CURRENT_LIST_DIR}/
)

