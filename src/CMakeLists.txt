set(PROJECT Server)

file(GLOB_RECURSE source_list "*.cpp" "*.hpp")

add_library(${PROJECT_NAME} SHARED ${source_list})

target_link_libraries(${PROJECT_NAME} PRIVATE
    OMP-SDK
)