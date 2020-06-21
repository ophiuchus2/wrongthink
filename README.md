# opensrcchat
Meant to be a real time chat application, with features similar to discord (a clone basically). It's a self hosted web application with the back end implementation in c++ with support for different clients.

## Features
* support a large number of concurrent websocket connections
* anonymous users + normal user accounts
* public rooms/channels listing on main page
* file sharing via webtorrent
* Allow users to create chat rooms/channels
* voice chat via webrtc mesh
* option for full p2p ring based group chats with webrtc

## tech

* `boost` - https://www.boost.org/
* `coroutine examples` - https://github.com/luncliff/coroutine
    * https://gist.github.com/MattPD/9b55db49537a90545a90447392ad3aeb
* `lib of coroutine abstractions` - https://github.com/lewissbaker/cppcoro
    * mit licensed
* `c++ http server lib` - https://github.com/titi38/libnavajo
    * http
    * ssl
    * websockets
    * static & dynamic pages
    * compile static pages into binary
    * license: CeCILL-C FREE SOFTWARE LICENSE AGREEMENT
        * some strange open source license
* `restbed` - https://github.com/Corvusoft/restbed - c++ http/websocket library for restful applications
    * AGPL licensed - [wiki](https://en.wikipedia.org/wiki/Affero_General_Public_License)
    * >This provision requires that the full source code be made available to any network user of the AGPL-licensed work, typically a web application. 
* **`uWebsockets` - https://github.com/uNetworking/uWebSockets**
    * Simple, secure & standards compliant web server for the most demanding of applications 
    * modern
    * regular updates
    * websockets & http
    * Apache License 2.0 - [wiki](https://en.wikipedia.org/wiki/Apache_License)
    * >The Apache License is a permissive free software license written by the Apache Software Foundation (ASF).[5] It allows users to use the software for any purpose, to distribute it, to modify it, and to distribute modified versions of the software under the terms of the license, without concern for royalties. The ASF and its projects release their software products under the Apache License. The license is also used by many non-ASF projects. 
* `mongodb c++ driver` - https://github.com/mongodb/mongo-cxx-driver
* `mongodb` - https://www.mongodb.com/
* `https://webtorrent.io/faq` - web torrent
* webrtc
    * https://github.com/pion/webrtc
    * https://github.com/webrtc/samples
    * https://www.html5rocks.com/en/tutorials/webrtc/basics/#toc-signaling
    * https://webrtc-security.github.io/
* https://gist.github.com/jo/8619441 - list of js crypto libs
* https://github.com/signalapp/libsignal-protocol-c
