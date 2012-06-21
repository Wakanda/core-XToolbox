cmake_minimum_required(VERSION 2.8)
project(KERNELIPC)


set(KernelIPCIncludeDir   ${KernelIPCRoot} PARENT_SCOPE)


file(GLOB Sources ${KernelIPCRoot}/Sources/V*.cpp
  ${KernelIPCRoot}/Sources/XLinux*.cpp
  ${KernelIPCRoot}/Sources/XPosix*.cpp
  ${KernelIPCRoot}/Sources/XSysV*.cpp)

#This file defines the components entry point ; It must be compiled
#with each component but is not part of KernelIpc shared library
list(REMOVE_ITEM Sources ${KernelIPCRoot}/Sources/VComponentLibrary.cpp)

set(ComponentEntryPoint  ${KernelIPCRoot}/Sources/VComponentLibrary.cpp PARENT_SCOPE)


add_library(KernelIPC SHARED ${Sources})


include_directories(${IcuIncludeDir} ${XBoxRoot})


target_link_libraries(KernelIPC Kernel)