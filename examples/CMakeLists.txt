cmake_minimum_required(VERSION 3.10)
project(deterministic_scheduler_examples VERSION 0.1)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
# Examples
find_package (deterministic_scheduler REQUIRED)
include_directories(${deterministic_scheduler_INCLUDE_DIR})
set(add_task_dir add_task)
# test task burst time
add_executable(deterministic_scheduler_test_burst_time 
${add_task_dir}/test_burst_time_main.cc 
${add_task_dir}/user_cpp_func_define.cc
${add_task_dir}/user_func_define.c
)
target_link_libraries(deterministic_scheduler_test_burst_time deterministic_scheduler)
target_include_directories(deterministic_scheduler_test_burst_time PRIVATE ${add_task_dir})
# add task 
add_executable(deterministic_scheduler_add_task 
${add_task_dir}/add_task_main.cc 
${add_task_dir}/user_cpp_func_define.cc
${add_task_dir}/user_func_define.c
)
target_link_libraries(deterministic_scheduler_add_task deterministic_scheduler)
target_include_directories(deterministic_scheduler_add_task PRIVATE ${add_task_dir})
# enable scheduler
set(enable_dir enable_scheduler)
add_executable(enable_deterministic_scheduler 
${enable_dir}/enable_scheduler_main.cc
)
target_link_libraries(enable_deterministic_scheduler deterministic_scheduler)
# print scheduler information
set(print_dir print_information )
add_executable(print_scheduler_information
${print_dir}/print_information_main.cc
)
target_link_libraries(print_scheduler_information deterministic_scheduler)















