cmake_minimum_required(VERSION 3.10)
project(BreakoutGame)

find_package(raylib REQUIRED)

# 编译测试程序
add_executable(collision_test collision_test.cpp)
target_link_libraries(collision_test raylib)

# 启用 CTest
enable_testing()
add_test(
  NAME collision_test
  COMMAND $<TARGET_FILE:collision_test>
)