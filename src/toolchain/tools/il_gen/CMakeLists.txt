cmake_minimum_required(VERSION 2.8)

add_executable(il_gen
    main.cpp
)

add_dependencies(il_gen core disa i0 langc flat)

target_link_libraries(il_gen core langc disa i0 flat boost_serialization)

install(TARGETS il_gen DESTINATION bin)

