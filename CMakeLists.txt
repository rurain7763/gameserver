cmake_minimum_required(VERSION 3.5)

project(asio_server)

set(CMAKE_CXX_STANDARD 17)

# 엔디안 확인
include(TestBigEndian)
test_big_endian(IS_BIG_ENDIAN)

if(IS_BIG_ENDIAN)
    add_definitions(-DBIG_ENDIAN)
else()
    add_definitions(-DLITTLE_ENDIAN)
endif()

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

    find_package(Boost 1.65.0 REQUIRED COMPONENTS serialization)

    add_library(common STATIC
        ./common/src/Packet.cpp
        ./common/src/Session.cpp
        ./common/src/TcpClient.cpp
        ./common/src/TcpServer.cpp
        ./common/src/UdpServer.cpp
    )

    target_include_directories(common
        PRIVATE ${BOOST_INCLUDEDIR}
        PRIVATE ./common/include
    )

    target_link_libraries(common
        Boost::serialization
    )

elseif(APPLE)
    set(BREW_DIR "" CACHE PATH "Path to Homebrew directory")

    if(NOT DEFINED BREW_DIR)
        message(FATAL_ERROR "Please set BREW_DIR to the path of Homebrew directory")
    endif()

    # Homebrew를 통해 Boost 설치
    execute_process(COMMAND brew install boost)

    find_package(Boost 1.65.0 REQUIRED COMPONENTS asio serialization)

    add_library(common STATIC
        ./common/src/Packet.cpp
        ./common/src/Session.cpp
        ./common/src/TcpClient.cpp
        ./common/src/TcpServer.cpp
        ./common/src/UdpServer.cpp
    )

    target_include_directories(common
        PRIVATE ${BREW_DIR}/include
        PRIVATE ./common/include
    )

    target_link_libraries(common
        ${BOOST_LIBRARYDIR}/libboost_serialization.a
    )

else()
    # apt-get을 통해 Boost 설치
    execute_process(COMMAND sudo apt-get install libboost-all-dev)

endif()

add_executable(server
    ./server/src/main.cpp
)

target_include_directories(server
    PRIVATE ./common/include
)

target_link_libraries(server
    common
)

if(WIN32)
    # boost_serialization으로 시작하는 DLL 파일 복사
    file(GLOB BOOST_DLLS "${VCPKG_INSTALLED}/bin/boost_serialization*.dll")
    foreach(BOOST_DLL ${BOOST_DLLS})
        add_custom_command(TARGET server POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${BOOST_DLL}
            $<TARGET_FILE_DIR:server>)
    endforeach()
endif()

add_executable(client
    ./client/src/main.cpp
)

target_include_directories(client
    PRIVATE ./common/include
)

target_link_libraries(client
    common
)

if(WIN32)
    # boost_serialization으로 시작하는 DLL 파일 복사
    file(GLOB BOOST_DLLS "${VCPKG_INSTALLED}/bin/boost_serialization*.dll")
    foreach(BOOST_DLL ${BOOST_DLLS})
        add_custom_command(TARGET client POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${BOOST_DLL}
            $<TARGET_FILE_DIR:client>)
    endforeach()
endif()

