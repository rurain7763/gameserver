cmake_minimum_required(VERSION 3.10)

project(CLIWrapper VERSION 1.0 LANGUAGES CXX)

if(WIN32)
    add_library(CLIWrapper SHARED
        src/TcpClientManager.cpp
        src/TcpClientWrapper.cpp
        src/TcpClientNative.cpp
    )

    target_include_directories(CLIWrapper
        PRIVATE ${BOOST_INCLUDEDIR}
        PRIVATE ../common/include
        PRIVATE ../vendor/msgpack-c/include
        PRIVATE include
    )

    target_link_directories(CLIWrapper
        PRIVATE ${BOOST_LIBRARYDIR}
    )

    target_link_libraries(CLIWrapper
        common
    )

    set_target_properties(CLIWrapper PROPERTIES COMMON_LANGUAGE_RUNTIME "")
endif()