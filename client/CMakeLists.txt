cmake_minimum_required(VERSION 3.5)

project(client VERSION 1.0 LANGUAGES CXX)

add_executable(client
    src/main.cpp
)

if(WIN32)
    target_include_directories(client
        PRIVATE ${BOOST_INCLUDEDIR}
        PRIVATE ../common/include
        PRIVATE ../vendor/msgpack-c/include
    )

    target_link_directories(client
		PRIVATE ${VCPKG_INSTALLED}/lib
	)

    # common DLL 파일 복사
    add_custom_command(TARGET client POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:common>
        $<TARGET_FILE_DIR:client>)

elseif(APPLE)
    target_include_directories(client
        PRIVATE ../common/include
        PRIVATE ${BREW_DIR}/include
    )

endif()

target_link_libraries(client
    common
)