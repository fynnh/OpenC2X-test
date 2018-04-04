set(test_SRCS
   test.cpp
)

add_executable (test_openc2x ${test_SRCS})
target_link_libraries (test_openc2x zmq protobuf boost_system boost_thread asn proto messages utility uci)
install(TARGETS test_openc2x DESTINATION bin)
