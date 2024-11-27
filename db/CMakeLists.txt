cmake_minimum_required(VERSION 3.5)

project(db VERSION 1.0 LANGUAGES CXX)

add_library(db SHARED
    src/MySqlClient.cpp
    src/MySqlResult.cpp
)

target_compile_definitions(db 
    PRIVATE DLL_EXPORT
)

if(WIN32)
    target_include_directories(db
        PRIVATE include
        PRIVATE ${BOOST_INCLUDEDIR}
        PRIVATE ${VENDOR_DIR}/boost-mysql/include
        PRIVATE ../common/include
    )

    target_link_directories(db
        PRIVATE ${BOOST_LIBRARYDIR}
    )

endif()

target_link_libraries(db 
    Boost::charconv 
    Threads::Threads 
    OpenSSL::Crypto 
    OpenSSL::SSL
)


