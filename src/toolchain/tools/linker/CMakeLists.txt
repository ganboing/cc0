cmake_minimum_required(VERSION 2.8)

add_executable(linker
    main.cpp
)

add_dependencies(linker core disa i0 langc flat)

target_link_libraries(linker core langc disa i0 flat boost_serialization)
#target_link_libraries(cc0 ${BOEHMGC_LIBRARIES})

install(TARGETS linker DESTINATION bin)

