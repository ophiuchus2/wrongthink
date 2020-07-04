# Wrongthink
Meant to be a real time chat application, with features similar to discord (a clone basically). It's a self hosted web application with the back end implementation in c++ with support for different clients.

**Matrix room**: #wrongthink:matrix.org

## Project files

* `wrongthink.cpp` - server implementation
* `test_client.cpp` - test showing a simple gRPC client implemented in c++
* `protocol/proto/wrongthink.proto` - protobuf datatype & RPC service definintions

## Building

### Third party libraries

* [gRPC](https://github.com/grpc/grpc) - gRPC is a modern, open source, high-performance remote procedure call (RPC) framework that can run anywhere
* [SOCI](https://github.com/SOCI/soci) - C++ Database Access Library
* [uWebsockets](https://github.com/uNetworking/uWebSockets) - Simple, secure & standards compliant web server for the most demanding of applications

### Prerequisites

Most of the dependencies are included in the build, they are located in the `third_party` directory. The only thing that needs to be installed separately is [Postgresql](https://www.postgresql.org/). The postgresql `libpq`  library is used by `SOCI` for database access, but this should be included by default in the Postgresql installation. You may need to tweak the include directories in the `CMakeLists.txt` file depending on where the libpq header files are installed on your system:

```
# include directories
target_include_directories(wrongthink PUBLIC
                           include
                           ${CMAKE_CURRENT_BINARY_DIR}
                           ${CMAKE_CURRENT_BINARY_DIR}/soci/include
                           /usr/local/include/
                           /usr/include/postgresql
                           )
```

Once postgresql is installed, the following needs to be done manually prior to running the wrongthink server:

1. Create a database user named `wrongthink` with password `test`
   1. The database username/password will come from a configuration file in the future
2. Create a new database named `wrongthink` that's owned by the wrongthink user

### Unix

1. clone the repository

`git clone https://github.com/ophiuchus2/wrongthink.git --recurse-submodules`

2. create cmake build directory

```
cd wrongthink
mkdir -p cmake/build
```

3. run cmake & build

```
cd cmake/build
cmake ../..
make
```

4. the build should complete successfully & the server + test binaries are now located in `cmake/build`

You should see the following files produced during the build:

* `wrongthink` - server binary
* `test_client` - client binary
* `wrongthink.grpc*` - grpc generated files
  * these include the c++ classes used for client/server communication
* `wrongthink.pb*` - protobuf generated files
  * these in include the c++ class definitions for the protobuf data structures

6. execute the server in one terminal and the client in another:

**Server output**

```
./wrongthink
wrongthink version: 0.1
Server listening on 0.0.0.0:50051
```

**Client output**

```
./test_client
got channel: channel 1
got channel: channel 2
got channel: channel 3
```

The client creates 3 channels on the server, then retrieves them & prints their names to the console.

## Features

* support a large number of concurrent connections
* anonymous users + normal user accounts
* public channels listing on main page
* file sharing via webtorrent
* Allow users to create chat rooms/channels
* voice chat via webrtc mesh
* option for full p2p ring based group chats with webrtc
