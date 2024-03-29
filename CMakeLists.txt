cmake_minimum_required(VERSION 2.6.0)

# Set the plugin name to build
project(pivottoopcua)

# Supported options:
# -DFLEDGE_INCLUDE
# -DFLEDGE_LIB
# -DFLEDGE_SRC
# -DFLEDGE_INSTALL
#
# If no -D options are given and FLEDGE_ROOT environment variable is set
# then Fledge libraries and header files are pulled from FLEDGE_ROOT path.

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Generation version header file
set_source_files_properties(version.h PROPERTIES GENERATED TRUE)
add_custom_command(
  OUTPUT version.h
  DEPENDS ${CMAKE_SOURCE_DIR}/VERSION
  COMMAND ${CMAKE_SOURCE_DIR}/mkversion ${CMAKE_SOURCE_DIR}
  COMMENT "Generating version header"
  VERBATIM
)
include_directories(${CMAKE_BINARY_DIR})

# Set plugin type (south, north, filter)
set(PLUGIN_TYPE "filter")

# Add here all needed Fledge libraries as list
set(NEEDED_FLEDGE_LIBS common-lib plugins-common-lib filters-common-lib)

# Find source files
file(GLOB SOURCES src/*.cpp src/*.c)

# Find Fledge includes and libs, by including FindFledge.cmak file
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})
find_package(Fledge)
# If errors: make clean and remove Makefile
if (NOT FLEDGE_FOUND)
    if (EXISTS "${CMAKE_BINARY_DIR}/Makefile")
        execute_process(COMMAND make clean WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
        file(REMOVE "${CMAKE_BINARY_DIR}/Makefile")
    endif()
    # Stop the build process
    message(FATAL_ERROR "Fledge plugin '${PROJECT_NAME}' build error.")
endif()
# On success, FLEDGE_INCLUDE_DIRS and FLEDGE_LIB_DIRS variables are set 

# Add ./include
include_directories(include)
# Add Fledge include dir(s)
include_directories(${FLEDGE_INCLUDE_DIRS})

# Add Fledge lib path
link_directories(${FLEDGE_LIB_DIRS})

# Add other include paths this plugin needs
if (FLEDGE_SRC)
    message(STATUS "Using third-party includes " ${FLEDGE_SRC}/C/thirdparty/Simple-Web-Server)
    include_directories(${FLEDGE_SRC}/C/thirdparty/Simple-Web-Server)
    message(STATUS "Using third-party includes " ${FLEDGE_SRC}/C/thirdparty/rapidjson)
    include_directories(${FLEDGE_SRC}/C/thirdparty/rapidjson/include)
    
    include_directories(${FLEDGE_SRC}/C/plugins/filter/common/include)
else()
    message(STATUS "FLEDGE_INCLUDE_DIRS= " ${FLEDGE_INCLUDE_DIRS})
    include_directories(${FLEDGE_INCLUDE_DIRS}/Simple-Web-Server)
endif()

# Add Fledge lib path
link_directories(${FLEDGE_LIB_DIRS})

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} -Wl,--no-as-needed")

# Create shared library
add_library(${PROJECT_NAME} SHARED ${SOURCES} version.h)

# Add Fledge library names
target_link_libraries(${PROJECT_NAME} ${NEEDED_FLEDGE_LIBS})

# Add additional libraries
target_link_libraries(${PROJECT_NAME} PRIVATE -lpthread  -ldl)

# Set the build version 
set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION 1)

set(FLEDGE_INSTALL "" CACHE INTERNAL "")
# Install library
if (FLEDGE_INSTALL)
	message(STATUS "Installing ${PROJECT_NAME} in ${FLEDGE_INSTALL}/plugins/${PLUGIN_TYPE}/${PROJECT_NAME}")
	install(TARGETS ${PROJECT_NAME} DESTINATION ${FLEDGE_INSTALL}/plugins/${PLUGIN_TYPE}/${PROJECT_NAME})
endif()

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Coverage")
  set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -O0 --coverage")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 --coverage")
  set(GCOVR_ADDITIONAL_ARGS "--exclude-unreachable-branches" "--exclude-throw-branches" )
  add_subdirectory(tests)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  set(CMAKE_CXX_FLAGS "-std=c++11 -O0")
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
  set(CMAKE_CXX_FLAGS "-std=c++11 -O3")
endif()

