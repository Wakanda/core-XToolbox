cmake_minimum_required(VERSION 2.8)
project(JSDEBUGGER)

 
set(JsDebuggerIncludeDir    ${JsDebuggerRoot}/Interfaces PARENT_SCOPE)


file(GLOB Sources ${JsDebuggerRoot}/Sources/*.cpp)


add_library(JsDebugger SHARED ${Sources})


include_directories(${IcuIncludeDir}
  ${InspectorDir}
  ${JsDebuggerRoot}/Headers
  ${JsDebuggerRoot}/Interfaces
  ${ServerNetIncludeDir}
  ${XBoxRoot}
  ${ComponentsRoot})


target_link_libraries(JsDebugger Kernel KernelIPC ServerNet)
