##    ▄                           ▄    █
##  ▄▄█▄▄   ▄▄▄    ▄▄▄▄   ▄▄▄   ▄▄█▄▄  █ ▄▄    ▄▄▄    ▄ ▄▄
##    █    █▀ ▀█  █▀ ▀█  █▀  █    █    █▀  █  █▀  █   █▀  ▀
##    █    █   █  █   █  █▀▀▀▀    █    █   █  █▀▀▀▀   █
##    ▀▄▄  ▀█▄█▀  ▀█▄▀█  ▀█▄▄▀    ▀▄▄  █   █  ▀█▄▄▀   █
##                 ▄  █
##                  ▀▀
                                                         

cmake_minimum_required(VERSION 3.13)

project(goolib LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (WIN32)
  set(PLATFORM                        "mingw")
elseif (APPLE)
  set(PLATFORM                        "darwin")
elseif (UNIX)
  set(PLATFORM                        "linux")
endif()
set(BUILD                             "build/${PLATFORM}")

################################################################################
##                                                                            ##
##                                   argparse                                 ##
##                                                                            ##
################################################################################
set(ARGPARSE_DIR                      "3rd/argparse-1.1.0")
set(ARGPARSE_INCLUDE_DIR              "${ARGPARSE_DIR}")
set(ARGPARSE_LIBRARY_DIR              "${ARGPARSE_DIR}/${BUILD}")
set(ARGPARSE_LIBRARIES                libargparse.a)

################################################################################
##                                                                            ##
##                                  GOOGLETEST                                ##
##                                                                            ##
################################################################################
set(GOOGLETEST_DIR                    "3rd/googletest-1.18.0")
set(GOOGLETEST_INCLUDE_DIR            "${GOOGLETEST_DIR}/googletest/include")
set(GOOGLETEST_LIBRARY_DIR            "${GOOGLETEST_DIR}/${BUILD}/lib")
set(GOOGLETEST_LIBRARIES              gtest gtest_main gmock gmock_main)

################################################################################
##                                                                            ##
##                                  together                                  ##
##                                                                            ##
################################################################################
set(TOGETHER_SOURCES
  "gen/together-poco.c"
  "gen/together-codec.c"
  "src/together-net.c"
  "src/together-util.c"
)

include_directories(
  "src"
  "gen"
  ${ARGPARSE_INCLUDE_DIR}
)

link_directories(
  ${ARGPARSE_LIBRARY_DIR}
  ${GOOGLETEST_LIBRARY_DIR}
)

################################################################################
##                                                                            ##
##                                     TOOL                                   ##
##                                                                            ##
################################################################################

add_library(together STATIC
  ${TOGETHER_SOURCES}
)
target_compile_definitions(together PRIVATE
  _POSIX_C_SOURCE=200809L
)

add_executable(together-server
  ${TOGETHER_SOURCES}
  "src/together-server.c"
)

add_executable(together-client
  ${TOGETHER_SOURCES}
  "src/together-client.c"
)

add_executable(together-make
  ${TOGETHER_SOURCES}
  "src/together-make.c"
)

if (WIN32)
  target_link_libraries(together-server PRIVATE
    -Wl,--start-group
    -Wl,--end-group

    ws2_32
    iphlpapi
    crypt32
    wldap32
    bcrypt
  )
elseif (APPLE)
  set_target_properties(together-server PROPERTIES
    MACOSX_RPATH TRUE
    BUILD_RPATH "@executable_path"
    INSTALL_RPATH "@executable_path"
    BUILD_WITH_INSTALL_RPATH FALSE
  )
elseif (UNIX)
endif()


################################################################################
##                                                                            ##
##                                     TEST                                   ##
##                                                                            ##
################################################################################

add_executable(together-codec-tests 
  ${TOGETHER_SOURCES}
  test/together-codec-tests.cpp
)

target_include_directories(together-codec-tests PRIVATE
  ${GOOGLETEST_INCLUDE_DIR}
)

target_link_libraries(together-codec-tests PRIVATE
    ${GOOGLETEST_LIBRARIES}
)

add_executable(together-e2e-simple 
  ${TOGETHER_SOURCES}
  test/together-e2e-simple.cpp
)

target_include_directories(together-e2e-simple PRIVATE
  ${GOOGLETEST_INCLUDE_DIR}
)

target_link_libraries(together-e2e-simple PRIVATE
    ${GOOGLETEST_LIBRARIES}
)

add_executable(together-e2e-busy 
  ${TOGETHER_SOURCES}
  test/together-e2e-busy.cpp
)

target_include_directories(together-e2e-busy PRIVATE
  ${GOOGLETEST_INCLUDE_DIR}
)

target_link_libraries(together-e2e-busy PRIVATE
    ${GOOGLETEST_LIBRARIES}
)

include(GoogleTest)
gtest_discover_tests(together-codec-tests)
gtest_discover_tests(together-e2e-simple)
gtest_discover_tests(together-e2e-busy)
