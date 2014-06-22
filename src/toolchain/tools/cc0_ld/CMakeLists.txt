cmake_minimum_required(VERSION 2.8)

add_executable(cc0_ld
    main.cpp
)

add_dependencies(cc0_ld core i0 disa flat langc)

target_link_libraries(cc0_ld core i0 disa flat langc boost_serialization)

install(TARGETS cc0_ld DESTINATION bin)

