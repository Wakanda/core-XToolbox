cmake_minimum_required(VERSION 2.8)
project(JAVASCRIPT)


set(JavaScriptIncludeDir    ${JavaScriptRoot}/Sources PARENT_SCOPE)


file(GLOB Sources ${JavaScriptRoot}/Sources/*.cpp)


add_library(JavaScript SHARED ${Sources})


include_directories(${CurlIncludeDir}
  ${Graphics}
  ${IcuIncludeDir}
  ${JavaScriptRoot}/Sources
  ${JsCoreIncludeDir}
  ${XBoxRoot}
  ${ServerNet}
  ${JavaScriptRoot}/../../../Components/Main )


target_link_libraries(JavaScript Curl Graphics JsCore Kernel KernelIPC ServerNet Xml ZLib)
