# Wrongthink
Meant to be a real time chat application, with features similar to discord (a clone basically). It's a self hosted web application with the back end implementation in c++ with support for different clients.

**Matrix**: #wrongthink:matrix.org

## Project files

* `wrongthink.cpp` - server implementation
* `test_client.cpp` - test showing a simple grpc client implemented in c++

## Building

### Unix

1. clone the repository

`git clone https://github.com/ophiuchus2/wrongthink.git`

2. initialize & update the submodules

`git submodule update --init --recursive`

3. create cmake build directory

`mkdir -p cmake/build`

4. run cmake & build

```
cd cmake/build
cmake ../..
make
```

5. the build should complete successfully & the server + test binaries are now located in `cmake/build`

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

* support a large number of concurrent websocket connections
* anonymous users + normal user accounts
* public rooms/channels listing on main page
* file sharing via webtorrent
* Allow users to create chat rooms/channels
* voice chat via webrtc mesh
* option for full p2p ring based group chats with webrtc
