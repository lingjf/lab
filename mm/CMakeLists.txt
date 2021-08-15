CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(mm)

ADD_EXECUTABLE(a.out 
    ia32_modrm_map.cpp 
    ia32_opcode_map.cpp 
    mini_disassembler.cpp 
    preamble_patcher.cpp
    preamble_patcher_with_stub.cpp
    preamble_patcher_test.cpp
)
