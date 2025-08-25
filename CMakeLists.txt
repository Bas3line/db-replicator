cmake_minimum_required(VERSION 3.16)
project(psql-cdc-replicator)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -flto -DNDEBUG")

find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(replicator ${SOURCES})

target_link_libraries(replicator ${PQXX_LIBRARIES})
target_include_directories(replicator PRIVATE ${PQXX_INCLUDE_DIRS})
target_compile_options(replicator PRIVATE ${PQXX_CFLAGS_OTHER})
