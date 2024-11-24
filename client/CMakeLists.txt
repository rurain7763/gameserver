cmake_minimum_required(VERSION 3.5)

project(client VERSION 1.0 LANGUAGES CXX)

add_executable(client
    src/main.cpp
)

if(WIN32)
    target_include_directories(client
        PRIVATE ../common/include
        PRIVATE ../vendor/msgpack-c/include
    )

    target_link_directories(client
		PRIVATE ${VCPKG_INSTALLED}/lib
	)

    # boost_serialization으로 시작하는 DLL 파일 복사
    file(GLOB BOOST_DLLS "${VCPKG_INSTALLED}/bin/boost_serialization*.dll")
    foreach(BOOST_DLL ${BOOST_DLLS})
        add_custom_command(TARGET client POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${BOOST_DLL}
            $<TARGET_FILE_DIR:client>)
    endforeach()

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