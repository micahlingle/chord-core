cmake_minimum_required(VERSION 3.16)

project(chord_core VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CHORD_CORE_BUILD_TESTS "Build unit tests" ON)
option(CHORD_CORE_ENABLE_COVERAGE "Build with line and branch coverage instrumentation" OFF)
set(CHORD_CORE_GCOVR_EXECUTABLE "gcovr" CACHE STRING "gcovr executable used by the coverage target")
set(CHORD_CORE_GCOV_EXECUTABLE "gcov" CACHE STRING "gcov-compatible executable used by gcovr")
set(CHORD_CORE_EVAL_MIN_ACCURACY "0.70" CACHE STRING "Default minimum overlap accuracy required by discovered dataset evaluation targets")
set(CHORD_CORE_DATASET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/datasets" CACHE PATH "Directory containing WAV files and matching .labels.csv files for automatic dataset evaluation")

find_program(CHORD_CORE_CLANG_FORMAT_EXECUTABLE
    NAMES
        clang-format
        clang-format-18
        clang-format-17
        clang-format-16
        clang-format-15
    REQUIRED
)

file(GLOB_RECURSE CHORD_CORE_FORMAT_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
)

if(APPLE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
    set(CHORD_CORE_GCOV_EXECUTABLE "xcrun llvm-cov gcov" CACHE STRING "gcov-compatible executable used by gcovr" FORCE)
endif()

file(GLOB CHORD_CORE_DATASET_WAV_FILES CONFIGURE_DEPENDS
    ${CHORD_CORE_DATASET_DIR}/*.wav
)

set(CHORD_CORE_DATASET_EVAL_WAV_FILES)
set(CHORD_CORE_DATASET_EVAL_LABEL_FILES)
set(CHORD_CORE_DATASET_EVAL_MIN_ACCURACIES)
foreach(dataset_wav ${CHORD_CORE_DATASET_WAV_FILES})
    get_filename_component(dataset_stem "${dataset_wav}" NAME_WE)
    set(dataset_labels "${CHORD_CORE_DATASET_DIR}/${dataset_stem}.labels.csv")
    if(EXISTS "${dataset_labels}")
        set(dataset_min_accuracy "${CHORD_CORE_EVAL_MIN_ACCURACY}")
        set(dataset_min_accuracy_file "${CHORD_CORE_DATASET_DIR}/${dataset_stem}.min_accuracy.txt")
        if(EXISTS "${dataset_min_accuracy_file}")
            file(READ "${dataset_min_accuracy_file}" dataset_min_accuracy)
            string(STRIP "${dataset_min_accuracy}" dataset_min_accuracy)
            if(NOT dataset_min_accuracy MATCHES "^(0(\\.[0-9]+)?|1(\\.0+)?)$")
                message(FATAL_ERROR
                    "Dataset accuracy override file ${dataset_min_accuracy_file} must contain a number in [0, 1].")
            endif()
        endif()

        list(APPEND CHORD_CORE_DATASET_EVAL_WAV_FILES "${dataset_wav}")
        list(APPEND CHORD_CORE_DATASET_EVAL_LABEL_FILES "${dataset_labels}")
        list(APPEND CHORD_CORE_DATASET_EVAL_MIN_ACCURACIES "${dataset_min_accuracy}")
    endif()
endforeach()

list(LENGTH CHORD_CORE_DATASET_EVAL_WAV_FILES CHORD_CORE_DATASET_EVAL_COUNT)

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
    src/evaluation.cpp
    src/fft_processor.cpp
    src/chroma_extractor.cpp
    src/chord_template_matcher.cpp
    src/chord_detector.cpp
    src/signal_analysis.cpp
    src/temporal_smoother.cpp
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

add_executable(chord_core_analyze
    src/analyze_main.cpp
)

target_link_libraries(chord_core_analyze
    PRIVATE
        chord_core
)

chord_core_enable_coverage(chord_core_analyze)

if(CHORD_CORE_DATASET_EVAL_COUNT GREATER 0)
    set(CHORD_CORE_DATASET_EVAL_COMMANDS)
    math(EXPR CHORD_CORE_DATASET_EVAL_LAST_INDEX "${CHORD_CORE_DATASET_EVAL_COUNT} - 1")
    foreach(dataset_index RANGE ${CHORD_CORE_DATASET_EVAL_LAST_INDEX})
        list(GET CHORD_CORE_DATASET_EVAL_WAV_FILES ${dataset_index} dataset_wav)
        list(GET CHORD_CORE_DATASET_EVAL_LABEL_FILES ${dataset_index} dataset_labels)
        list(GET CHORD_CORE_DATASET_EVAL_MIN_ACCURACIES ${dataset_index} dataset_min_accuracy)
        get_filename_component(dataset_stem "${dataset_wav}" NAME_WE)
        list(APPEND CHORD_CORE_DATASET_EVAL_COMMANDS
            COMMAND chord_core_validate_labels
                --labels "${dataset_labels}"
            COMMAND chord_core_eval
                --wav "${dataset_wav}"
                --labels "${dataset_labels}"
                --min-accuracy "${dataset_min_accuracy}"
        )
    endforeach()

    add_executable(chord_core_validate_labels
        src/validate_labels_main.cpp
    )

    target_link_libraries(chord_core_validate_labels
        PRIVATE
            chord_core
    )

    chord_core_enable_coverage(chord_core_validate_labels)

    add_executable(chord_core_eval
        src/eval_main.cpp
    )

    target_link_libraries(chord_core_eval
        PRIVATE
            chord_core
    )

    chord_core_enable_coverage(chord_core_eval)

    add_custom_target(dataset_eval
        ALL
        DEPENDS chord_core_validate_labels chord_core_eval
        ${CHORD_CORE_DATASET_EVAL_COMMANDS}
        COMMENT "Running automatic dataset evaluation"
        VERBATIM
    )
endif()

add_custom_target(format
    ALL
    COMMAND ${CHORD_CORE_CLANG_FORMAT_EXECUTABLE} -i ${CHORD_CORE_FORMAT_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Formatting C++ sources with clang-format"
    VERBATIM
)

add_dependencies(chord_core format)

if(CHORD_CORE_BUILD_TESTS)
    include(CTest)
    if(BUILD_TESTING)
        find_package(GTest REQUIRED)

        add_executable(chord_core_tests
            tests/test_audio_loader.cpp
            tests/test_chroma_extractor.cpp
            tests/test_chord_detector.cpp
            tests/test_chord_template_matcher.cpp
            tests/test_evaluation.cpp
            tests/test_fft_processor.cpp
            tests/test_signal_analysis.cpp
            tests/test_temporal_smoother.cpp
        )

        target_link_libraries(chord_core_tests
            PRIVATE
                chord_core
                GTest::gtest_main
        )

        chord_core_enable_coverage(chord_core_tests)

        include(GoogleTest)
        gtest_discover_tests(chord_core_tests)

        if(CHORD_CORE_DATASET_EVAL_COUNT GREATER 0 AND TARGET chord_core_eval)
            foreach(dataset_wav dataset_labels dataset_min_accuracy IN ZIP_LISTS
                    CHORD_CORE_DATASET_EVAL_WAV_FILES
                    CHORD_CORE_DATASET_EVAL_LABEL_FILES
                    CHORD_CORE_DATASET_EVAL_MIN_ACCURACIES)
                get_filename_component(dataset_stem "${dataset_wav}" NAME_WE)
                string(REGEX REPLACE "[^A-Za-z0-9_]" "_" dataset_name "${dataset_stem}")
                add_test(
                    NAME chord_core_dataset_validate_${dataset_name}
                    COMMAND chord_core_validate_labels
                        --labels "${dataset_labels}"
                )
                add_test(
                    NAME chord_core_dataset_eval_${dataset_name}
                    COMMAND chord_core_eval
                        --wav "${dataset_wav}"
                        --labels "${dataset_labels}"
                        --min-accuracy "${dataset_min_accuracy}"
                )
            endforeach()
        endif()

        if(CHORD_CORE_ENABLE_COVERAGE)
            set(CHORD_CORE_COVERAGE_DEPENDENCIES chord_core_tests)
            if(TARGET chord_core_validate_labels)
                list(APPEND CHORD_CORE_COVERAGE_DEPENDENCIES chord_core_validate_labels)
            endif()
            if(TARGET chord_core_eval)
                list(APPEND CHORD_CORE_COVERAGE_DEPENDENCIES chord_core_eval)
            endif()

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
                DEPENDS ${CHORD_CORE_COVERAGE_DEPENDENCIES}
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                COMMENT "Running unit tests and generating line/branch coverage"
                VERBATIM
            )
        endif()
    endif()
endif()
