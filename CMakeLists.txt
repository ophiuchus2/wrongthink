cmake_minimum_required(VERSION 3.10)

# set the project name
project(wrongthink VERSION 0.1)

# set project version configuration file
configure_file(include/WrongthinkConfig.h.in WrongthinkConfig.h)

set(CMAKE_BUILD_TYPE Debug)

IF(APPLE)
  SET(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build architectures for Mac OS X" FORCE)
  #set(CMAKE_APPLE_SILICON_PROCESSOR "arm64")
ENDIF(APPLE)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(WITH_DB2 OFF)
set(WITH_FIREBIRD OFF)
set(WITH_MYSQL OFF)
set(WITH_ODBC OFF)
set(WITH_ORACLE OFF)
set(WITH_POSTGRESQL ON)
set(WITH_SQLITE3 ON)

add_definitions(-D_GNU_SOURCE -DBOOST_STACKTRACE_USE_ADDR2LINE)

find_package(Threads REQUIRED)
INCLUDE_DIRECTORIES( third_party/boost_uuid )

add_subdirectory(third_party/soci ${CMAKE_CURRENT_BINARY_DIR}/soci EXCLUDE_FROM_ALL)
add_subdirectory(third_party/spdlog ${CMAKE_CURRENT_BINARY_DIR}/spdlog EXCLUDE_FROM_ALL)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(third_party/googletest ${CMAKE_CURRENT_BINARY_DIR}/googletest EXCLUDE_FROM_ALL)

# JUST
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error -w")

set(GRPC_AS_SUBMODULE TRUE)

if(GRPC_AS_SUBMODULE)
  # One way to build a projects that uses gRPC is to just include the
  # entire gRPC project tree via "add_subdirectory".
  # This approach is very simple to use, but the are some potential
  # disadvantages:
  # * it includes gRPC's CMakeLists.txt directly into your build script
  #   without and that can make gRPC's internal setting interfere with your
  #   own build.
  # * depending on what's installed on your system, the contents of submodules
  #   in gRPC's third_party/* might need to be available (and there might be
  #   additional prerequisites required to build them). Consider using
  #   the gRPC_*_PROVIDER options to fine-tune the expected behavior.
  #
  # A more robust approach to add dependency on gRPC is using
  # cmake's ExternalProject_Add (see cmake_externalproject/CMakeLists.txt).

  # Include the gRPC's cmake build (normally grpc source code would live
  # in a git submodule called "third_party/grpc", but this example lives in
  # the same repository as gRPC sources, so we just look a few directories up)
  add_subdirectory(third_party/grpc ${CMAKE_CURRENT_BINARY_DIR}/grpc EXCLUDE_FROM_ALL)
  message(STATUS "Using gRPC via add_subdirectory.")

  # After using add_subdirectory, we can now use the grpc targets directly from
  # this build.
  set(_PROTOBUF_LIBPROTOBUF libprotobuf)
  set(_REFLECTION grpc++_reflection)
  if(CMAKE_CROSSCOMPILING)
    find_program(_PROTOBUF_PROTOC protoc)
  else()
    set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
  endif()
  set(_GRPC_GRPCPP grpc++)
  if(CMAKE_CROSSCOMPILING)
    find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
  else()
    set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)
  endif()
elseif(GRPC_FETCHCONTENT)
  # Another way is to use CMake's FetchContent module to clone gRPC at
  # configure time. This makes gRPC's source code available to your project,
  # similar to a git submodule.
  message(STATUS "Using gRPC via add_subdirectory (FetchContent).")
  include(FetchContent)
  FetchContent_Declare(
    grpc
    GIT_REPOSITORY https://github.com/grpc/grpc.git
    # when using gRPC, you will actually set this to an existing tag, such as
    # v1.25.0, v1.26.0 etc..
    # For the purpose of testing, we override the tag used to the commit
    # that's currently under test.
    GIT_TAG        vGRPC_TAG_VERSION_OF_YOUR_CHOICE)
  FetchContent_MakeAvailable(grpc)

  # Since FetchContent uses add_subdirectory under the hood, we can use
  # the grpc targets directly from this build.
  set(_PROTOBUF_LIBPROTOBUF libprotobuf)
  set(_REFLECTION grpc++_reflection)
  set(_PROTOBUF_PROTOC $<TARGET_FILE:protoc>)
  set(_GRPC_GRPCPP grpc++)
  if(CMAKE_CROSSCOMPILING)
    find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
  else()
    set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)
  endif()
else()
  # This branch assumes that gRPC and all its dependencies are already installed
  # on this system, so they can be located by find_package().

  # Find Protobuf installation
  # Looks for protobuf-config.cmake file installed by Protobuf's cmake installation.
  set(protobuf_MODULE_COMPATIBLE TRUE)
  find_package(Protobuf CONFIG REQUIRED)
  message(STATUS "Using protobuf ${protobuf_VERSION}")

  set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
  set(_REFLECTION gRPC::grpc++_reflection)
  set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)

  # Find gRPC installation
  # Looks for gRPCConfig.cmake file installed by gRPC's cmake installation.
  find_package(gRPC CONFIG REQUIRED)
  message(STATUS "Using gRPC ${gRPC_VERSION}")

  set(_GRPC_GRPCPP gRPC::grpc++)
  set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
endif()

# Proto file
get_filename_component(wt_proto "protocol/proto/wrongthink.proto" ABSOLUTE)
get_filename_component(wt_proto_path "${wt_proto}" PATH)

# Generated sources
set(wt_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/wrongthink.pb.cc")
set(wt_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/wrongthink.pb.h")
set(wt_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/wrongthink.grpc.pb.cc")
set(wt_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/wrongthink.grpc.pb.h")
add_custom_command(
      OUTPUT "${wt_proto_srcs}" "${wt_proto_hdrs}" "${wt_grpc_srcs}" "${wt_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${wt_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${wt_proto}"
      DEPENDS "${wt_proto}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories("third_party/spdlog/include")

configure_file(third_party/soci/include/soci/soci.h ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(third_party/soci/include/soci/postgresql/soci-postgresql.h ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(third_party/soci/include/soci/sqlite3/soci-sqlite3.h ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

add_executable(wrongthink "wrongthink.cpp"
  "SynchronizedChannel.cpp"
  "WrongthinkServiceImpl.cpp"
  "DB/DBInterface.cpp"
  "DB/DBPostgres.cpp"
  "DB/DBSQLite.cpp"
  ${wt_proto_srcs}
  ${wt_grpc_srcs})

target_link_libraries(wrongthink
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF}
  soci_core
  soci_postgresql
  soci_sqlite3
  pq
  ${Boost_LIBRARIES}
  spdlog::spdlog_header_only
  dl)

add_executable(test_client "test_client.cpp"
  ${wt_proto_srcs}
  ${wt_grpc_srcs})

target_link_libraries(test_client
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

# build tests
add_executable(tests "test/rpc_tests.cpp"
  "SynchronizedChannel.cpp"
  "WrongthinkServiceImpl.cpp"
  "DB/DBInterface.cpp"
  "DB/DBPostgres.cpp"
  "DB/DBSQLite.cpp"
  ${wt_proto_srcs}
  ${wt_grpc_srcs})

target_link_libraries(tests
  gtest_main
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF}
  soci_core
  soci_postgresql
  soci_sqlite3
  pq
  ${Boost_LIBRARIES}
  spdlog::spdlog_header_only
  dl)

target_include_directories(tests PUBLIC
                           include
                           ${CMAKE_CURRENT_BINARY_DIR}
                           ${CMAKE_CURRENT_BINARY_DIR}/soci/include
                           /usr/local/include/
                           /usr/include/postgresql
                           .)

target_link_directories(tests PUBLIC
                      /opt/homebrew/opt/libpq/lib) 

target_compile_definitions(tests PUBLIC GTEST)

enable_testing()
add_test(NAME TestRpc COMMAND tests)

# include directories
target_include_directories(wrongthink PUBLIC
                           include
                           ${CMAKE_CURRENT_BINARY_DIR}
                           ${CMAKE_CURRENT_BINARY_DIR}/soci/include
                           /usr/local/include/
                           /usr/include/postgresql
                           )

target_link_directories(wrongthink PUBLIC
                      /opt/homebrew/opt/libpq/lib) 

# include directories
target_include_directories(test_client PUBLIC
                          include
                          ${CMAKE_CURRENT_BINARY_DIR}
                          )
