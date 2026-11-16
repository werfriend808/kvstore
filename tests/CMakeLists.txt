add_executable(kvstore_tests
  smoke_test.cc
)
target_link_libraries(kvstore_tests PRIVATE kv_warnings GTest::gtest_main)

gtest_discover_tests(kvstore_tests DISCOVERY_TIMEOUT 30)
