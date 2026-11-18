add_executable(kvstore_tests
  smoke_test.cc
  status_test.cc
  kvstore_test.cc
  record_test.cc
)
target_link_libraries(kvstore_tests PRIVATE kvstore kv_warnings GTest::gtest_main)
# Internal headers (src/record.h) are tested directly.
target_include_directories(kvstore_tests PRIVATE ${PROJECT_SOURCE_DIR}/src)

gtest_discover_tests(kvstore_tests DISCOVERY_TIMEOUT 30)
