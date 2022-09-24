cmake_minimum_required(VERSION 3.6)

set(CMAKE_C_STANDARD 99)
project(zero_patch)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -shared -w -m32 -D_GNU_SOURCE")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY /home/rfx/zero)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY /home/rfx/zero)
set(SOURCE_FILES
    main.c
    deadlock.c
    deadlock.h
    dog.c
    dog.h
    gfx.c
    gfx.h
    hdd.c
    hdd.h
    sound.c
    sound.h
    ticket.c
    ticket.h
    util.c
    util.h fs.h fs.c hooks.c hooks.h mk6io.c mk6io.h usb.h)



add_executable(zero_patch.so ${SOURCE_FILES})

target_link_libraries(zero_patch.so dl pthread usb config)
