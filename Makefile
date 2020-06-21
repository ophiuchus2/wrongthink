EXAMPLE_FILES := HelloWorld EchoServer BroadcastingEchoServer UpgradeSync UpgradeAsync
THREADED_EXAMPLE_FILES := HelloWorldThreaded EchoServerThreaded
PROGRAM_FILE := wrongthink
override CXXFLAGS += -lpthread -Wconversion -std=c++17 -Iinclude -I./uWebSockets/src -IuWebSockets/uSockets/src -I uWebSockets/examples/helpers
override LDFLAGS += uWebSockets/uSockets/*.o -lz

.PHONY: all
all:
	$(CXX) -pthread -flto -O3 $(CXXFLAGS) src/*.cpp -o bin/$(PROGRAM_FILE) $(LDFLAGS)

clean:
	rm -rf bin/*
