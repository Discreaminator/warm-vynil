# FetchContent for Catch2 v3, pocketfft, dr_wav (SPEC §2 / §11)
include(FetchContent)

set(WG_CPM_SOURCE_CACHE "${CMAKE_BINARY_DIR}/_deps_cache" CACHE PATH "Local dependency cache")

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
    GIT_SHALLOW    TRUE
)

# Header-only helpers used later; M0 only needs Catch2 for smoke test.
# pocketfft / dr_wav fetched when needed by tests / wg_render.

FetchContent_Declare(
    dr_wav
    GIT_REPOSITORY https://github.com/mackron/dr_libs.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)

FetchContent_Declare(
    pocketfft
    GIT_REPOSITORY https://github.com/mreineck/pocketfft.git
    GIT_TAG        cpp
    GIT_SHALLOW    TRUE
)

if(WG_BUILD_TESTS)
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
endif()

FetchContent_MakeAvailable(dr_wav)
FetchContent_MakeAvailable(pocketfft)

add_library(dr_wav INTERFACE)
target_include_directories(dr_wav INTERFACE "${dr_wav_SOURCE_DIR}")

add_library(pocketfft INTERFACE)
target_include_directories(pocketfft INTERFACE "${pocketfft_SOURCE_DIR}")
