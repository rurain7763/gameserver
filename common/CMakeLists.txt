cmake_minimum_required(VERSION 3.5)

project(common VERSION 1.0 LANGUAGES CXX)

# 엔디안 확인
include(TestBigEndian)
test_big_endian(IS_BIG_ENDIAN)

if(IS_BIG_ENDIAN)
    add_definitions(-DBIG_ENDIAN)
else()
    add_definitions(-DLITTLE_ENDIAN)
endif()

add_library(common SHARED
    src/Packet.cpp
    src/Session.cpp
    src/TcpClient.cpp
    src/TcpServer.cpp
    src/UdpServer.cpp
)

target_compile_definitions(common 
    PRIVATE DLL_EXPORT
)

if(WIN32) 
    target_include_directories(common
        PRIVATE ${BOOST_INCLUDEDIR}
        PRIVATE include
    )
    
elseif(APPLE)
    target_include_directories(common
        PRIVATE ${BREW_DIR}/include
        PRIVATE include
    )

    target_link_directories(common
        PRIVATE ${BREW_DIR}/lib
    )

endif()

target_link_libraries(common
    Boost::serialization
)