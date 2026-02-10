# doctest (header-only)
add_library(doctest INTERFACE)
target_include_directories(doctest INTERFACE ${CMAKE_SOURCE_DIR}/src/dependencies/doctest)

# doctest CMake integration (per-test-case discovery for ctest)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(doctest)

add_executable(tests
    test_main.cpp
    test_crop_single_tile.cpp
    test_endian.cpp
    test_assign_tile_id.cpp
    test_tiles_lst.cpp
    test_proto_files.cpp
    test_frm_write.cpp
    test_palettize.cpp
    test_maps_txt.cpp
    dat2/test_dat2_io.cpp
    dat2/test_dat2_parse.cpp
    dat2/test_dat2_extract.cpp
    dat2/test_dat2_writer.cpp
)
target_link_libraries(tests PRIVATE
    FO2MapEditLib
    doctest
    glad
    imgui_docking
    STB
    glfw
    ImFileDialog
)
target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/src/QFO2Tool)
doctest_discover_tests(tests)
