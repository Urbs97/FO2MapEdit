add_library(int2ssl_lib STATIC
  Namespace.cpp
  Node.cpp
  Utility.cpp
  FalloutScript.cpp
  FalloutScriptDecompile.cpp
  FalloutScriptDefObject.cpp
  FalloutScriptDump.cpp
  FalloutScriptStore.cpp
  OpcodeAttributes.cpp
  Opcode.cpp
  ProcTable.cpp
  StartupCode.cpp
  XGetopt.cpp
)

target_include_directories(int2ssl_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Third-party code — don't treat warnings as errors
set_target_properties(int2ssl_lib PROPERTIES COMPILE_WARNING_AS_ERROR OFF)
