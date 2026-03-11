cmake_minimum_required(VERSION 3.10)
project(MultiApps)

option(BUILD_CHAT_SERVER "Build Chat Server" ON)

if(BUILD_CHAT_SERVER)
    message(STATUS "Building Chat Server ...")
    include(${CMAKE_SOURCE_DIR}/ChatServerCMakeLists.txt)
endif()