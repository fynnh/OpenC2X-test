set(denm_app_SRCS
   denmApp.cpp
)

find_package(cppzmq)
if(NOT cppzmq_FOUND)
    message([FATAL_ERROR] "zermoq missing")
endif()

include_directories(${cppzmq_INCLUDE_DIR})
add_executable (denmApp ${denm_app_SRCS})
target_link_libraries(denmApp protobuf boost_system boost_thread ${cppzmq_LIBRARY} asn proto messages utility uci)
install(TARGETS denmApp DESTINATION bin)
