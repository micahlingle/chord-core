cmake_minimum_required(VERSION 3.16)

project(chord_core VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CHORD_CORE_BUILD_TESTS "Build unit tests" ON)
option(CHORD_CORE_ENABLE_COVERAGE "Build with line and branch coverage instrumentation" OFF)
set(CHORD_CORE_GCOVR_EXECUTABLE "gcovr" CACHE STRING "gcovr executable used by the coverage target")
set(CHORD_CORE_GCOV_EXECUTABLE "gcov" CACHE STRING "gcov-compatible executable used by gcovr")

if(APPLE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
    set(CHORD_CORE_GCOV_EXECUTABLE "xcrun llvm-cov gcov" CACHE STRING "gcov-compatible executable used by gcovr" FORCE)
endif()

function(chord_core_enable_coverage target_name)
    if(CHORD_CORE_ENABLE_COVERAGE)
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
            target_compile_options(${target_name} PRIVATE -O0 -g --coverage)
            target_link_options(${target_name} PRIVATE --coverage)
        else()
            message(FATAL_ERROR "Coverage is only supported for GCC/Clang-style compilers")
        endif()
    endif()
endfunction()

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

find_package(FFTW3f QUIET CONFIG)
find_package(FFTW3 QUIET CONFIG)
if(NOT TARGET FFTW::fftw3f)
    if(TARGET FFTW3::fftw3f)
        add_library(FFTW::fftw3f INTERFACE IMPORTED)
        set_target_properties(FFTW::fftw3f PROPERTIES
            INTERFACE_LINK_LIBRARIES FFTW3::fftw3f
        )
    elseif(TARGET FFTW3f::fftw3f)
        add_library(FFTW::fftw3f INTERFACE IMPORTED)
        set_target_properties(FFTW::fftw3f PROPERTIES
            INTERFACE_LINK_LIBRARIES FFTW3f::fftw3f
        )
    else()
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(PC_FFTW3F QUIET fftw3f)
        endif()

        find_path(FFTW3F_INCLUDE_DIR
            NAMES fftw3.h
            HINTS
                ${PC_FFTW3F_INCLUDE_DIRS}
                /opt/homebrew/include
                /opt/homebrew/opt/fftw/include
                /usr/local/include
        )

        find_library(FFTW3F_LIBRARY
            NAMES fftw3f libfftw3f
            HINTS
                ${PC_FFTW3F_LIBRARY_DIRS}
                /opt/homebrew/lib
                /opt/homebrew/opt/fftw/lib
                /usr/local/lib
        )

        if(NOT FFTW3F_INCLUDE_DIR OR NOT FFTW3F_LIBRARY)
            message(FATAL_ERROR "FFTW single-precision library was not found. Install FFTW and/or set CMAKE_PREFIX_PATH.")
        endif()

        add_library(FFTW::fftw3f UNKNOWN IMPORTED)
        set_target_properties(FFTW::fftw3f PROPERTIES
            IMPORTED_LOCATION "${FFTW3F_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${FFTW3F_INCLUDE_DIR}"
        )
    endif()
endif()

add_library(chord_core
    src/audio_loader.cpp
    src/fft_processor.cpp
    src/chroma_extractor.cpp
    src/chord_detector.cpp
    src/signal_analysis.cpp
)

target_include_directories(chord_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(chord_core
    PUBLIC
        SndFile::sndfile
        FFTW::fftw3f
)

target_compile_options(chord_core
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

chord_core_enable_coverage(chord_core)

add_executable(chord_core_demo
    src/main.cpp
)

target_link_libraries(chord_core_demo
    PRIVATE
        chord_core
)

chord_core_enable_coverage(chord_core_demo)

if(CHORD_CORE_BUILD_TESTS)
    include(CTest)
    if(BUILD_TESTING)
        find_package(GTest REQUIRED)

        add_executable(chord_core_tests
            tests/test_audio_loader.cpp
            tests/test_chord_detector.cpp
            tests/test_fft_processor.cpp
            tests/test_signal_analysis.cpp
        )

        target_link_libraries(chord_core_tests
            PRIVATE
                chord_core
                GTest::gtest_main
        )

        chord_core_enable_coverage(chord_core_tests)

        include(GoogleTest)
        gtest_discover_tests(chord_core_tests)

        if(CHORD_CORE_ENABLE_COVERAGE)
            add_custom_target(coverage
                COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR} --output-on-failure
                COMMAND ${CHORD_CORE_GCOVR_EXECUTABLE}
                    --root ${CMAKE_SOURCE_DIR}
                    --object-directory ${CMAKE_BINARY_DIR}
                    --filter "src/.*"
                    --filter "include/.*"
                    --exclude "tests/.*"
                    --txt-metric branch
                    --gcov-executable "${CHORD_CORE_GCOV_EXECUTABLE}"
                    --print-summary
                    --html-details ${CMAKE_BINARY_DIR}/coverage.html
                    ${CMAKE_BINARY_DIR}
                DEPENDS chord_core_tests
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                COMMENT "Running unit tests and generating line/branch coverage"
                VERBATIM
            )
        endif()
    endif()
endif()
