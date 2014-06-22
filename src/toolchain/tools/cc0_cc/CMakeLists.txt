cmake_minimum_required(VERSION 2.8)

add_executable(cc0_cc
    main.cpp
)

add_dependencies(cc0_cc core langc)

target_link_libraries(cc0_cc core langc boost_serialization)

install(TARGETS cc0_cc DESTINATION bin)

