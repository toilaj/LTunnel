set(SRC_DIR main.c client.c server.c io.c device.c)
set(INC_DIR ../include /usr/local/include)
set(LINK_DIR /usr/local/lib)
include_directories(${INC_DIR})
link_directories(${LINK_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(LTunnel ${SRC_DIR})
target_link_libraries(LTunnel ${LINK_DIR}/libev.a)

