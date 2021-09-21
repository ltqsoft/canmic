cmake_minimum_required(VERSION 3.18)


#-- Preprocess ----------------------------------------------------------------------------

set( CMAKE_CXX_STANDARD             11              )
set( CMAKE_CXX_STANDARD_REQUIRED    True            )
set( CMAKE_FIND_LIBRARY_PREFIXES    "lib"           )
set( CMAKE_FIND_LIBRARY_SUFFIXES    ".a;.dll.a;.so" )

set ( DEFAULT_CMAKE_INSTALL_PREFIX_PENGU "/usr/local"   )
set ( DEFAULT_CMAKE_INSTALL_PREFIX_WIN32 "C:/dev"       )

if( ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR} )
    message( FATAL_ERROR "Binary directory should not be the same as source directory, please specify a different one!" )
endif()

set( OPENAL_LIB_NAME "openal" )
if( WIN32 )
    set( OPENAL_LIB_NAME "OpenAL32" )
endif() #WIN32

find_library( FIND_ICONV_RESULT       "iconv"       NAMES "iconv" )


#-- CMake Options -------------------------------------------------------------------------

# set(SYSTEM_INCLUDE_PATH "What" CACHE STRING "System include path" )
if( WIN32 )
    set( WIN32_CENTRALIZED_PACKAGES_PATH "C:/dev" CACHE STRING "If you follow linux library structure, you can specify include/lib path for all of them here." )
endif()


#-- Project and targets -----------------------------------------------------------------

project(canmic VERSION 1.0)

set( GLOBAL_INCLUDE_PATH
    #
    "${WIN32_CENTRALIZED_PACKAGES_PATH}/include"
    )
set( GLOBAL_LIBRARY_PATH
    #
    "/usr/local/lib"
    "${WIN32_CENTRALIZED_PACKAGES_PATH}/lib"
    )

include_directories(
    "${GLOBAL_INCLUDE_PATH}"
    "${PROJECT_SOURCE_DIR}/include"
    )
link_directories( ${GLOBAL_LIBRARY_PATH} )


# targets/canmic
add_executable( canmic
    main.cc
    include/strdef.hh
    )

target_include_directories(canmic PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
    )

target_link_libraries( canmic
    PUBLIC iconv
    )

#-- Install section ---------------------------------------------------------------------

# empty

#-- Print configure result --------------------------------------------------------------

message( STATUS "Install location: ${CMAKE_INSTALL_PREFIX}" )
message( STATUS "inc-path: ${GLOBAL_INCLUDE_PATH}" )
message( STATUS "lib-path: ${GLOBAL_LIBRARY_PATH}" )
if( WIN32 )
    message( STATUS "win32/inc-path: ${WIN32_CENTRALIZED_INCLUDE_PATH}" )
    message( STATUS "win32/lib-path: ${WIN32_CENTRALIZED_LIBRARY_PATH}" )
endif() #WIN32
