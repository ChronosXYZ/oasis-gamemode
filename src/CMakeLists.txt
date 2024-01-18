set(PROJECT Server)

file(GLOB_RECURSE source_list CONFIGURE_DEPENDS "./*.cpp" "./*.hpp")

# configure_gettext(
#     DOMAIN "domain-name"
#     TARGET_NAME "oasis"
#     SOURCES ${source_list}
#     POTFILE_DESTINATION "locale"
#     LANGUAGES "en_US.UTF-8")

find_package(libpqxx REQUIRED)
find_package(spdlog REQUIRED)

add_library(${PROJECT_NAME} SHARED ${source_list})

target_link_libraries(${PROJECT_NAME} PRIVATE
    OMP-SDK
    tinygettext
    libpqxx::pqxx
    spdlog::spdlog
)