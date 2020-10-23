# some definitions
add_compile_definitions(APP_NAME="MiniVM Eeyore Interpreter")
add_compile_definitions(APP_VERSION="${PROJECT_VERSION}")
add_compile_definitions(APP_VERSION_MAJOR=${PROJECT_VERSION_MAJOR})
add_compile_definitions(APP_VERSION_MINOR=${PROJECT_VERSION_MINOR})
add_compile_definitions(APP_VERSION_PATCH=${PROJECT_VERSION_PATCH})

# project include directories
include_directories(.)
# include_directories(${PROJECT_SOURCE_DIR}/3rdparty/xstl)

# all of C++ source files
file(GLOB_RECURSE SOURCES "*.cpp")

# executable
add_executable(Eeyore ${SOURCES})
