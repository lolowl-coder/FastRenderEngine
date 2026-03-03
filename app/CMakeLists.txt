add_executable(FREApp
    main.cpp
)

target_link_libraries(FREApp
    PRIVATE FRE
)

target_include_directories(FREApp
    PRIVATE ${CMAKE_SOURCE_DIR}/fre/include
)

set_target_properties(FREApp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

set_target_properties(FREApp PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)