cmake_minimum_required(VERSION 3.5)

project(gameserver VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

if(WIN32)
    set(VCPKG_DIR "" CACHE PATH "Path to vcpkg directory")

    if(NOT DEFINED VCPKG_DIR)
        message(FATAL_ERROR "Please set VCPKG_DIR to the path of vcpkg directory")
    endif()

    # vcpkg를 통해 Boost 설치
    execute_process(COMMAND ${CMAKE_SOURCE_DIR}/vcpkg/vcpkg install boost-asio boost-serialization
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/vcpkg)

    set(VCPKG_INSTALLED "${VCPKG_DIR}/installed/x64-windows")
    set(BOOST_INCLUDEDIR "${VCPKG_INSTALLED}/include")
    set(BOOST_LIBRARYDIR "${VCPKG_INSTALLED}/lib")

elseif(APPLE)
    set(BREW_DIR "" CACHE PATH "Path to Homebrew directory")

    if(NOT DEFINED BREW_DIR)
        message(FATAL_ERROR "Please set BREW_DIR to the path of Homebrew directory")
    endif()

    # Homebrew를 통해 Boost 설치
    execute_process(COMMAND brew install boost)

else()
    # apt-get을 통해 Boost 설치
    execute_process(COMMAND sudo apt-get install libboost-all-dev)

endif()

find_package(Boost 1.86.0 REQUIRED COMPONENTS serialization)
    
# 하위 디렉토리 추가
add_subdirectory(common)
add_subdirectory(cli)
add_subdirectory(server)
add_subdirectory(client)