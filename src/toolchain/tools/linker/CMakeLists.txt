cmake_minimum_required(VERSION 2.8)

add_executable(linker
    main.cpp
    ExportDeriveExpressions.cpp
    ExportDeriveTypes.cpp
)

add_dependencies(linker core)

target_link_libraries(linker core boost_serialization)
#target_link_libraries(cc0 ${BOEHMGC_LIBRARIES})

install(TARGETS linker DESTINATION bin)

