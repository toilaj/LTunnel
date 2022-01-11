set(SRC main.c client.c server.c io.c device.c)
include_directories(../include)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(LTunnel ${SRC})

