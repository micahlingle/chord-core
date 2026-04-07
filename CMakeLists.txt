cmake_minimum_required(VERSION 3.16)

project(chord_core VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CHORD_CORE_BUILD_TESTS "Build unit tests" ON)

find_package(SndFile QUIET CONFIG)
if(NOT TARGET SndFile::sndfile)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(PC_SNDFILE QUIET sndfile)
    endif()

    find_path(SNDFILE_INCLUDE_DIR
        NAMES sndfile.h
        HINTS
            ${PC_SNDFILE_INCLUDE_DIRS}
            /opt/homebrew/include
            /opt/homebrew/opt/libsndfile/include
            /usr/local/include
    )

    find_library(SNDFILE_LIBRARY
        NAMES sndfile libsndfile
        HINTS
            ${PC_SNDFILE_LIBRARY_DIRS}
            /opt/homebrew/lib
            /opt/homebrew/opt/libsndfile/lib
            /usr/local/lib
    )

    if(NOT SNDFILE_INCLUDE_DIR OR NOT SNDFILE_LIBRARY)
        message(FATAL_ERROR "libsndfile was not found. Install it and/or set CMAKE_PREFIX_PATH.")
    endif()

    add_library(SndFile::sndfile UNKNOWN IMPORTED)
    set_target_properties(SndFile::sndfile PROPERTIES
        IMPORTED_LOCATION "${SNDFILE_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${SNDFILE_INCLUDE_DIR}"
    )
endif()
add_library(chord_core
    src/audio_loader.cpp
    src/fft_processor.cpp
    src/chroma_extractor.cpp
    src/chord_detector.cpp
)

target_include_directories(chord_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(chord_core
    PUBLIC
        SndFile::sndfile
)

target_compile_options(chord_core
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

add_executable(chord_core_demo
    src/main.cpp
)

target_link_libraries(chord_core_demo
    PRIVATE
        chord_core
)

if(CHORD_CORE_BUILD_TESTS)
    include(CTest)
    if(BUILD_TESTING)
        find_package(GTest REQUIRED)

        add_executable(chord_core_tests
            tests/test_chord_detector.cpp
        )

        target_link_libraries(chord_core_tests
            PRIVATE
                chord_core
                GTest::gtest_main
        )

        include(GoogleTest)
        gtest_discover_tests(chord_core_tests)
    endif()
endif()
