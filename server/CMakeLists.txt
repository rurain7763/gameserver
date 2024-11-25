cmake_minimum_required(VERSION 3.5)

project(server VERSION 1.0 LANGUAGES CXX)

add_executable(server
    src/main.cpp
)

if(WIN32)
    target_include_directories(server
        PRIVATE ${BOOST_INCLUDEDIR}
        PRIVATE ../common/include
        PRIVATE ../vendor/msgpack-c/include
    )

    target_link_directories(server
		PRIVATE ${VCPKG_INSTALLED}/lib
	)

    # common DLL 파일 복사
    add_custom_command(TARGET server POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:common>
        $<TARGET_FILE_DIR:server>)

elseif(APPLE)
    target_include_directories(server
        PRIVATE ../common/include
        PRIVATE ${BREW_DIR}/include
    )

endif()

target_link_libraries(server
    common
)