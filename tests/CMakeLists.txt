add_executable(kvstore_tests
  smoke_test.cc
  status_test.cc
  kvstore_test.cc
)
target_link_libraries(kvstore_tests PRIVATE kvstore kv_warnings GTest::gtest_main)

gtest_discover_tests(kvstore_tests DISCOVERY_TIMEOUT 30)
