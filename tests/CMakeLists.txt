add_executable(wg_tests
    test_smoke.cpp
)

target_link_libraries(wg_tests PRIVATE
    wg_dsp
    Catch2::Catch2WithMain
)

wg_set_warnings(wg_tests)

include(Catch)
catch_discover_tests(wg_tests)
