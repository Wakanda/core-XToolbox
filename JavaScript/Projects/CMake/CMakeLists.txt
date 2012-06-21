cmake_minimum_required(VERSION 2.8)
project(JAVASCRIPT)


set(JavaScriptIncludeDir    ${JavaScriptRoot}/Sources PARENT_SCOPE)


file(GLOB Sources ${JavaScriptRoot}/Sources/*.cpp)

list(REMOVE_ITEM Sources ${JavaScriptRoot}/Sources/VJSRuntime_Image.cpp)


add_library(JavaScript SHARED ${Sources})


include_directories(${CurlIncludeDir}
  ${IcuIncludeDir}
  ${JavaScriptRoot}/Sources
  ${JsCoreIncludeDir}
  ${XBoxRoot}
  ${ServerNet})


target_link_libraries(JavaScript Curl JsCore Kernel KernelIPC Xml ZLib ServerNet)
