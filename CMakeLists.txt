cmake_minimum_required(VERSION 3.19)
project(oasis-gm LANGUAGES C CXX VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# build as 32-bit library
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

include(cmake/conan.cmake)

# conan_cmake_configure(REQUIRES boost/1.83.0 
#                       GENERATORS CMakeDeps
#                       OPTIONS "boost:without_atomic=False"
#                       OPTIONS "boost:without_chrono=False"
#                       OPTIONS "boost:without_container=False"
#                       OPTIONS "boost:without_context=True"
#                       OPTIONS "boost:without_contract=True"
#                       OPTIONS "boost:without_coroutine=True"
#                       OPTIONS "boost:without_date_time=False"
#                       OPTIONS "boost:without_exception=False"
#                       OPTIONS "boost:without_fiber=True"
#                       OPTIONS "boost:without_filesystem=True"
#                       OPTIONS "boost:without_graph=True"
#                       OPTIONS "boost:without_graph_parallel=True"
#                       OPTIONS "boost:without_iostreams=True"
#                       OPTIONS "boost:without_json=True"
#                       OPTIONS "boost:without_locale=False"
#                       OPTIONS "boost:without_log=True"
#                       OPTIONS "boost:without_math=True"
#                       OPTIONS "boost:without_mpi=True"
#                       OPTIONS "boost:without_nowide=True"
#                       OPTIONS "boost:without_program_options=True"
#                       OPTIONS "boost:without_python=True"
#                       OPTIONS "boost:without_random=True"
#                       OPTIONS "boost:without_regex=True"
#                       OPTIONS "boost:without_serialization=True"
#                       OPTIONS "boost:without_stacktrace=True"
#                       OPTIONS "boost:without_system=False"
#                       OPTIONS "boost:without_test=True"
#                       OPTIONS "boost:without_thread=False"
#                       OPTIONS "boost:without_timer=True"
#                       OPTIONS "boost:without_type_erasure=True"
#                       OPTIONS "boost:without_url=True"
#                       OPTIONS "boost:without_wave=True")

conan_cmake_configure(REQUIRES 
                            libpqxx/7.8.1
                            spdlog/1.13.0
                        GENERATORS CMakeDeps)

conan_cmake_autodetect(settings ARCH x86)

conan_cmake_install(PATH_OR_REFERENCE .
                    BUILD missing
                    REMOTE conancenter
                    PROFILE_BUILD default
                    SETTINGS ${settings})


add_subdirectory(vendor/sdk)
add_subdirectory(vendor/network)
add_subdirectory(vendor/tinygettext)
add_subdirectory(src)


