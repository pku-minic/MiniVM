cmake_minimum_required(VERSION 3.13)
project(MiniVM VERSION "0.1.3")

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
  # disable warnings caused by old version of Flex
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
option(LEGACY_MODE "enable legacy mode" OFF)
if(LEGACY_MODE)
  message("Legacy mode has been enabled.")
  add_compile_definitions(VM_LEGACY_MODE)
endif()

# no debugger: disable the built-in debugger of MiniVM
option(NO_DEBUGGER "disable the built-in debugger" OFF)
if(NO_DEBUGGER)
  message("The built-in debugger (MiniDbg) has been disabled.")
  add_compile_definitions(NO_DEBUGGER)
endif()

# find Flex/Bison
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

# find Readline
if(NOT NO_DEBUGGER)
  find_package(Readline)
endif()

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
if(NOT NO_DEBUGGER)
  include_directories(${Readline_INCLUDE_DIR})
endif()

# all of C++ source files
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE DEBUGGER_SRCS "src/debugger/*.cpp")
if (NO_DEBUGGER)
  list(REMOVE_ITEM SOURCES ${DEBUGGER_SRCS})
endif()
set(SOURCES ${SOURCES} ${FLEX_EeyoreLexer_OUTPUTS}
    ${BISON_EeyoreParser_OUTPUT_SOURCE})
set(SOURCES ${SOURCES} ${FLEX_TiggerLexer_OUTPUTS}
    ${BISON_TiggerParser_OUTPUT_SOURCE})

# executable
add_executable(minivm ${SOURCES})
if(NOT NO_DEBUGGER)
  target_link_libraries(minivm ${Readline_LIBRARY})
endif()
