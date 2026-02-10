add_library(FO2MapEditLib
  App_State.h
  App_State.cpp
  Edit_Animation.cpp
  Edit_Animation.h
  Image2Texture.cpp
  Image2Texture.h
  Load_Animation.cpp
  Load_Animation.h
  Load_Files.cpp
  Load_Files.h
  load_FRM_OpenGL.cpp
  load_FRM_OpenGL.h
  Load_Settings.cpp
  Load_Settings.h
  Save_Files.cpp
  Save_Files.h
  stb_impl.c
  Stroke_State.h
  Stroke_State.cpp
)

add_subdirectory(rendering)
add_subdirectory(formats)
add_subdirectory(worldmap)
add_subdirectory(platform)
add_subdirectory(file_types)
add_subdirectory(dat2)
add_subdirectory(ui)

#these need to be located after add_library()
#target (FO2MapEditLib) needs to be declared
#_BEFORE_ being added to compile defs
# Windows compile stuff
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
target_compile_definitions(
  FO2MapEditLib
  PUBLIC QFO2_WINDOWS
)

# Linux compile stuff
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
target_compile_definitions(
  FO2MapEditLib
  PUBLIC QFO2_LINUX
)
endif()

target_link_libraries(FO2MapEditLib PUBLIC ZLIB::ZLIB)
target_link_libraries(FO2MapEditLib PRIVATE int2ssl_lib)

target_include_directories(
  FO2MapEditLib SYSTEM PUBLIC
  ../dependencies/GLAD/include
  ../dependencies/tinyfiledialogs
  ../dependencies/imgui-1.90.8-docking
  ../dependencies/GLFW/glfw-3.4/include/
  ../dependencies/stb
  ../dependencies/ImFileDialog
  ../dependencies/int2ssl
)
