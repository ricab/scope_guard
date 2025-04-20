cmake_minimum_required(VERSION 3.11)

project(
    scope_guard
    VERSION 1.1.0
    LANGUAGES CXX
)

include(GNUInstallDirs)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(SCOPE_GUARD_MASTER_PROJECT ON)
endif()

add_library(scope_guard INTERFACE)
add_library(scope_guard::scope_guard ALIAS scope_guard)

target_include_directories(scope_guard
    INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

option(SCOPE_GUARD_INSTALL "Generate the install targets" ${SCOPE_GUARD_MASTER_PROJECT})
if(SCOPE_GUARD_INSTALL)
    include(CMakePackageConfigHelpers)

    install(
        TARGETS scope_guard
        EXPORT ${PROJECT_NAME}-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    install(FILES "scope_guard.hpp" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    set(config_file "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake")
    set(version_file "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake")
    set(config_install_destination "lib/cmake/${PROJECT_NAME}")

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.cmake.in"
        ${config_file}
        INSTALL_DESTINATION ${config_install_destination}
    )

    write_basic_package_version_file(
        ${version_file}
        COMPATIBILITY SameMajorVersion
    )

    install(
        FILES ${config_file} ${version_file}
        DESTINATION ${config_install_destination}
    )
    install(
        EXPORT ${PROJECT_NAME}-targets
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION ${config_install_destination}
    )
endif()

if(SCOPE_GUARD_MASTER_PROJECT)
    include(CTest)

    if(BUILD_TESTING)
        add_subdirectory(tests)
    endif()
endif()
