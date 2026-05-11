# CMake generated Testfile for 
# Source directory: D:/prog/C++/FollowerList
# Build directory: D:/prog/C++/FollowerList/build_test
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(AllTests "D:/prog/C++/FollowerList/build_test/unit_tests.exe")
set_tests_properties(AllTests PROPERTIES  _BACKTRACE_TRIPLES "D:/prog/C++/FollowerList/CMakeLists.txt;106;add_test;D:/prog/C++/FollowerList/CMakeLists.txt;0;")
add_test(IntegrationTests "D:/prog/C++/FollowerList/build_test/integration_tests.exe")
set_tests_properties(IntegrationTests PROPERTIES  _BACKTRACE_TRIPLES "D:/prog/C++/FollowerList/CMakeLists.txt;135;add_test;D:/prog/C++/FollowerList/CMakeLists.txt;0;")
subdirs("_deps/googletest-build")
