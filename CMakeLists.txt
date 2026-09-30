cmake_minimum_required(VERSION 3.16)

project(mini_task_runner LANGUAGES CXX)

add_executable(taskrunner
	src/main.cpp
	src/worker.cpp
)

target_include_directories(taskrunner PRIVATE include)
