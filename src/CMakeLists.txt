set(PROJECT Server)

file(GLOB_RECURSE source_list CONFIGURE_DEPENDS "./*.cpp" "./*.hpp")

find_package(libpqxx REQUIRED)
find_package(spdlog REQUIRED)

add_library(${PROJECT_NAME} SHARED ${source_list})

target_link_libraries(${PROJECT_NAME} PRIVATE
    OMP-SDK
    tinygettext
    libpqxx::pqxx
    spdlog::spdlog
)