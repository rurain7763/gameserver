cmake_minimum_required(VERSION 3.5)

project(db VERSION 1.0 LANGUAGES CXX)

add_library(db SHARED
    src/MySql.cpp
)

target_compile_definitions(db 
    PRIVATE DLL_EXPORT
    PRIVATE STATIC_CONCPP
)

if(WIN32)
    target_include_directories(db
        PRIVATE include
        PRIVATE ${BOOST_INCLUDEDIR}
        PRIVATE ${VENDOR_DIR}/mysql/include/jdbc
        PRIVATE ../common/include
    )

    target_link_directories(db
        PRIVATE ${BOOST_LIBRARYDIR}
    )
    
    target_link_libraries(db 
        debug ${VENDOR_DIR}/mysql/lib/debug/mysqlcppconn-static.lib
        optimized ${VENDOR_DIR}/mysql/lib/release/mysqlcppconn-static.lib
    )

endif()


