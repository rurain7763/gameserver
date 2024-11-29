cmake_minimum_required(VERSION 3.5)

project(server VERSION 1.0 LANGUAGES CXX)

add_executable(server
    src/main.cpp
    src/LobyServer.cpp
    src/ChatServer.cpp
    src/DatabaseServer.cpp
    src/Resources.cpp
)

if(WIN32)
    target_include_directories(server
        PRIVATE ${BOOST_INCLUDEDIR}
        PRIVATE ../common/include
        PRIVATE ../db/include
        PRIVATE include
        PRIVATE ${VENDOR_DIR}/boost-mysql/include
        PRIVATE ${VENDOR_DIR}/msgpack-c/include
    )

    target_link_directories(server
        PRIVATE ${BOOST_LIBRARYDIR}
	)

    # common DLL 파일 복사
    add_custom_command(TARGET server POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:common>
        $<TARGET_FILE_DIR:server>)

    # db DLL 파일 복사
    add_custom_command(TARGET server POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:db>
        $<TARGET_FILE_DIR:server>)

elseif(APPLE)
    target_include_directories(server
        PRIVATE ../common/include
        PRIVATE ${BREW_DIR}/include
    )

endif()

target_link_libraries(server
    common
    db
)