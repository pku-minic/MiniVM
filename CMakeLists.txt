cmake_minimum_required(VERSION 3.13)
project(MiniVM VERSION "0.1.1")

# set CMake module path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# # set compiler path
# set(CMAKE_C_COMPILER "/usr/local/opt/gcc/bin/gcc-10")
# set(CMAKE_CXX_COMPILER "/usr/local/opt/gcc/bin/g++-10")
# set(CMAKE_CXX_COMPILER_ID "GNU")

# C++17 standard support
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# enable all warnings and treat them as errors
if(MSVC)
  add_compile_options(/W3 /WX)
else()
  add_compile_options(-Wall -Werror)
endif()

# disable warnings caused by old version of Flex
if(MSVC)
  # TODO
else()
  add_compile_options(-Wall -Werror -Wno-register)
endif()

# definitions about version information
add_compile_definitions(APP_NAME="MiniVM Eeyore/Tigger Virtual Machine")
add_compile_definitions(APP_VERSION="${PROJECT_VERSION}")
add_compile_definitions(APP_VERSION_MAJOR=${PROJECT_VERSION_MAJOR})
add_compile_definitions(APP_VERSION_MINOR=${PROJECT_VERSION_MINOR})
add_compile_definitions(APP_VERSION_PATCH=${PROJECT_VERSION_PATCH})

# definitions about build option
# legacy mode: don't complain about symbol redefinition
option(MINIVM_LEGACY_MODE "enable legacy mode" OFF)
if(MINIVM_LEGACY_MODE)
  add_compile_definitions(VM_LEGACY_MODE)
endif()

# find Flex/Bison
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

# find Readline
find_package(Readline)

# generate lexer/parser of Eeyore
flex_target(EeyoreLexer src/front/eeyore.l
            ${CMAKE_CURRENT_BINARY_DIR}/eeyore.lexer.cpp
            COMPILE_FLAGS "-P eeyore_")
bison_target(EeyoreParser src/front/eeyore.y
             ${CMAKE_CURRENT_BINARY_DIR}/eeyore.parser.cpp
             COMPILE_FLAGS "-p eeyore_")
add_flex_bison_dependency(EeyoreLexer EeyoreParser)

# generate lexer/parser of Tigger
flex_target(TiggerLexer src/front/tigger.l
            ${CMAKE_CURRENT_BINARY_DIR}/tigger.lexer.cpp
            COMPILE_FLAGS "-P tigger_")
bison_target(TiggerParser src/front/tigger.y
             ${CMAKE_CURRENT_BINARY_DIR}/tigger.parser.cpp
             COMPILE_FLAGS "-p tigger_")
add_flex_bison_dependency(TiggerLexer TiggerParser)

# project include directories
include_directories(src)
include_directories(3rdparty/xstl)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${Readline_INCLUDE_DIR})

# all of C++ source files
file(GLOB_RECURSE SOURCES "src/*.cpp")
set(SOURCES ${SOURCES} ${FLEX_EeyoreLexer_OUTPUTS}
    ${BISON_EeyoreParser_OUTPUT_SOURCE})
set(SOURCES ${SOURCES} ${FLEX_TiggerLexer_OUTPUTS}
    ${BISON_TiggerParser_OUTPUT_SOURCE})

# executable
add_executable(minivm ${SOURCES})
target_link_libraries(minivm ${Readline_LIBRARY})
