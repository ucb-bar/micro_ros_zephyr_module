# SPDX-License-Identifier: Apache-2.0

if(CONFIG_MICROROS)

set(MICROROS_DIR ${ZEPHYR_CURRENT_MODULE_DIR})

# zephyr-chipyard-sw local: tie inner colcon-build state to the outer Zephyr
# build-dir lifecycle.
#
# The libmicroros makefile writes its outputs into MICROROS_DIR (inside the
# submodule) — both the slow caches (cloned ROS sources, ament tools install)
# and the fast outputs (build/install/log dirs, libmicroros.a, headers,
# configured_colcon.meta, zephyr_toolchain.cmake). `west build -p` only wipes
# CMAKE_BINARY_DIR, so the inner colcon CMakeCache.txts persist across
# pristine builds — masking changes to compile flags / toolchain config.
#
# Detect a freshly-pristine outer build via the absence of a stamp file in
# CMAKE_BINARY_DIR. When pristine, wipe the inner build/install/log + final
# artifacts so colcon re-configures with current flags. Preserve the
# expensive caches (micro_ros_src/src, micro_ros_dev) which take minutes to
# regenerate and don't carry stale state.
set(_microros_outer_stamp "${CMAKE_BINARY_DIR}/.microros_inner_state.stamp")
if(NOT EXISTS "${_microros_outer_stamp}")
    message(STATUS "micro-ROS: pristine outer build — wiping inner colcon state at ${MICROROS_DIR}")
    foreach(_p
        "${MICROROS_DIR}/micro_ros_src/build"
        "${MICROROS_DIR}/micro_ros_src/install"
        "${MICROROS_DIR}/micro_ros_src/log"
        "${MICROROS_DIR}/libmicroros.a"
        "${MICROROS_DIR}/include"
        "${MICROROS_DIR}/configured_colcon.meta"
        "${MICROROS_DIR}/zephyr_toolchain.cmake")
        if(EXISTS "${_p}")
            file(REMOVE_RECURSE "${_p}")
        endif()
    endforeach()
    file(WRITE "${_microros_outer_stamp}" "outer-build-tracked\n")
endif()

if(CMAKE_GENERATOR STREQUAL "Unix Makefiles")
set(submake "$(MAKE)")
else()
set(submake "make")
endif()

# micro-ROS library

zephyr_get_include_directories_for_lang_as_string(       C includes)
zephyr_get_system_include_directories_for_lang_as_string(C system_includes)
zephyr_get_compile_definitions_for_lang_as_string(       C definitions)
zephyr_get_compile_options_for_lang_as_string(           C options)

zephyr_get_include_directories_for_lang_as_string(       CXX includes_cxx)
zephyr_get_system_include_directories_for_lang_as_string(CXX system_includes_cxx)
zephyr_get_compile_definitions_for_lang_as_string(       CXX definitions_cxx)
zephyr_get_compile_options_for_lang_as_string(           CXX options_cxx)

set(external_project_cflags
    "${includes} ${definitions} ${options} ${system_includes}"
    )

set(external_project_cxxflags
    "${includes_cxx} ${definitions_cxx} ${options_cxx} ${system_includes_cxx}"
    )

# zephyr-chipyard-sw local: inject SDK 1.0.0-beta1 + picolibc compat shim
# into the colcon meta-build's CFLAGS (see zephyr_sdk_compat.h for rationale).
# The compat-includes dir provides shimmed Zephyr headers that moved between
# Zephyr 4.1 (which upstream tests against) and 4.2 (this repo).
set(_microros_compat_header "${CMAKE_CURRENT_SOURCE_DIR}/zephyr_sdk_compat.h")
set(_microros_compat_includes "${CMAKE_CURRENT_SOURCE_DIR}/zephyr_compat_includes")
if(EXISTS "${_microros_compat_header}")
    set(_microros_compat_cflags
        "-D__STDC_WANT_LIB_EXT1__=0 -include ${_microros_compat_header} -I${_microros_compat_includes}"
        )
    set(external_project_cflags
        "${external_project_cflags} ${_microros_compat_cflags}"
        )
    set(external_project_cxxflags
        "${external_project_cxxflags} ${_microros_compat_cflags}"
        )
endif()

include(ExternalProject)

externalproject_add(libmicroros_project
    PREFIX     ${CMAKE_BINARY_DIR}/libmicroros-prefix
    SOURCE_DIR ${MICROROS_DIR}
    BINARY_DIR ${MICROROS_DIR}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND
        ${submake} -f libmicroros.mk
            X_CFLAGS=${external_project_cflags}
            X_CXXFLAGS=${external_project_cxxflags}
            X_CC=${CMAKE_C_COMPILER}
            X_AR=${CMAKE_AR}
            X_RANLIB=${CMAKE_RANLIB}
            X_CXX=${CMAKE_CXX_COMPILER}
            COMPONENT_PATH=${MICROROS_DIR}
            ZEPHYR_BASE=${ZEPHYR_BASE}
            PROJECT_BINARY_DIR=${PROJECT_BINARY_DIR}
    INSTALL_COMMAND ""
    BUILD_BYPRODUCTS ${MICROROS_DIR}/libmicroros.a
    )

zephyr_link_libraries(${MICROROS_DIR}/libmicroros.a)

zephyr_interface_library_named(microros)
add_dependencies(microros libmicroros_project)
target_include_directories(microros INTERFACE ${MICROROS_DIR}/include)

execute_process(
    COMMAND
        make -f libmicroros.mk get_package_names
            COMPONENT_PATH=${MICROROS_DIR}
    WORKING_DIRECTORY
        ${MICROROS_DIR}
    OUTPUT_VARIABLE
        INCLUDE_ROS2_PACKAGES
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

foreach(pkg ${INCLUDE_ROS2_PACKAGES})
    target_include_directories(microros INTERFACE ${MICROROS_DIR}/include/${pkg})
endforeach()

# micro-ROS transport library

if(CONFIG_MICROROS_TRANSPORT_SERIAL)
    set(MICROROS_TRANSPORT_DIR ${MICROROS_DIR}/microros_transports/serial)
elseif(CONFIG_MICROROS_TRANSPORT_SERIAL_USB)
    set(MICROROS_TRANSPORT_DIR ${MICROROS_DIR}/microros_transports/serial-usb)
elseif(CONFIG_MICROROS_TRANSPORT_UDP)
    set(MICROROS_TRANSPORT_DIR ${MICROROS_DIR}/microros_transports/udp)
else()
    message(FATAL_ERROR Please set a micro-ROS transport)
endif()

zephyr_library_named(microros_transports)
zephyr_include_directories(${MICROROS_DIR}/include)
zephyr_include_directories(${MICROROS_TRANSPORT_DIR})

zephyr_library_sources(
   ${MICROROS_TRANSPORT_DIR}/microros_transports.c
)

add_dependencies(microros microros_transports)
add_dependencies(microros_transports libmicroros_project)

# Cleaning

set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
    ${MICROROS_DIR}/include
    ${MICROROS_DIR}/configured_colcon.meta
    ${MICROROS_DIR}/zephyr_toolchain.cmake
    )

endif()
