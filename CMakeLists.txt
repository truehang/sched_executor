cmake_minimum_required(VERSION 3.10)
project(deterministic_scheduler VERSION 0.1)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
# message("hh"${CMAKE_CURRENT_SOURCE_DIR})
set(schedule_include_dir  ${CMAKE_CURRENT_LIST_DIR}/include)
include_directories( ${schedule_include_dir} ${CMAKE_CURRENT_LIST_DIR}/core)

# Core library
add_library(deterministic_scheduler SHARED core/posix_shm.c core/func_interface.c 
core/func_interface.cc core/internal_pthread.c core/cpu_model.c 
core/pq_shm.c core/timestamp.c core/preprocess.cpp core/internal_sched.c)
target_link_libraries(deterministic_scheduler pthread rt)

# TEST
add_executable(cpu_model_test test/cpu_model_test.c)
target_link_libraries(cpu_model_test deterministic_scheduler)
add_executable(func_interface_invoker_test test/func_interface_invoker_test.cc)
add_executable(pq_shm_test test/pq_shm_test.c)
target_link_libraries(pq_shm_test deterministic_scheduler)
add_executable(external_signal_maker test/external_signal_maker.c)
target_link_libraries(external_signal_maker pthread)
# for gtest
# add_executable(preprocess_test test/preprocess_test.cpp)
# target_link_libraries(preprocess_test PRIVATE GTest::gtest_main deterministic_scheduler)
# gtest_discover_tests(preprocess_test)

#Tools
# -e: enable scheduler
# -p: print scheduler information
set(tools_dir ${CMAKE_CURRENT_LIST_DIR}/tools)
add_executable(
    deterministic_scheduler_tool
    ${tools_dir}/deterministic_scheduler_tool_main.cc
)
target_link_libraries(deterministic_scheduler_tool deterministic_scheduler)

        
# sudo make install
message("install path: " ${CMAKE_INSTALL_PREFIX})
install(TARGETS deterministic_scheduler deterministic_scheduler_tool
EXPORT  deterministic_scheduler-targets
                RUNTIME DESTINATION bin
                LIBRARY DESTINATION lib
                ARCHIVE DESTINATION lib)
# install header files
install(DIRECTORY include/ DESTINATION include/deterministic_scheduler)

install(EXPORT deterministic_scheduler-targets FILE deterministic_scheduler.cmake
NAMESPACE EM::
DESTINATION lib/cmake/deterministic_scheduler
)

set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/include/deterministic_scheduler
    CACHE PATH "Location of header files")
include(CMakePackageConfigHelpers)
configure_package_config_file(deterministic_scheduler-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/deterministic_scheduler-config.cmake
  INSTALL_DESTINATION lib/cmake/deterministic_scheduler
  PATH_VARS INCLUDE_INSTALL_DIR 
)

write_basic_package_version_file(
${CMAKE_CURRENT_BINARY_DIR}/deterministic_scheduler-config-version.cmake
  VERSION 0.1.0
  COMPATIBILITY SameMajorVersion)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/deterministic_scheduler-config.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/deterministic_scheduler-config-version.cmake"
DESTINATION lib/cmake/deterministic_scheduler
)

# Create a lib reference under /usr/lib
execute_process (
    COMMAND bash -c "sudo ln -s ${CMAKE_INSTALL_PREFIX}/lib/libdeterministic_scheduler.so /usr/lib/libdeterministic_scheduler.so"  
)














