cmake_minimum_required(VERSION 2.8)

add_executable(il_cc
    main.cpp
)

add_dependencies(il_cc core disa i0 langc flat)

target_link_libraries(il_cc core langc disa i0 flat boost_serialization boost_filesystem boost_system)

install(TARGETS il_cc DESTINATION bin)

