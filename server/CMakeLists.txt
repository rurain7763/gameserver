cmake_minimum_required(VERSION 3.5)

project(server VERSION 1.0 LANGUAGES CXX)

add_executable(server
    src/main.cpp
)

if(WIN32)
    target_include_directories(server
        PRIVATE ../common/include
    )

    # boost_serialization으로 시작하는 DLL 파일 복사
    file(GLOB BOOST_DLLS "${VCPKG_INSTALLED}/bin/boost_serialization*.dll")
    foreach(BOOST_DLL ${BOOST_DLLS})
        add_custom_command(TARGET server POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${BOOST_DLL}
            $<TARGET_FILE_DIR:server>)
    endforeach()

elseif(APPLE)
    target_include_directories(server
        PRIVATE ../common/include
        PRIVATE ${BREW_DIR}/include
    )

endif()

target_link_libraries(server
    common
)

