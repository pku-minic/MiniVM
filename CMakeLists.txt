cmake_minimum_required(VERSION 3.5)
project(MiniVM VERSION "0.0.1")

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
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  add_compile_options(-Wall -Wno-init-list-lifetime -Werror)
else()
  add_compile_options(-Wall -Werror)
endif()

# find Flex/Bison
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

# all sub-directories
add_subdirectory(src/eeyore)
