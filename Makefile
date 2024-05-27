CXX = g++
CXXFLAGS = -Wall
TARGET = InMemFileSystem
OBJS = InMemFileSystem.cpp Encryption.cpp Filesystem.cpp
LIBS = `pkg-config fuse --cflags --libs` -lssl -lcrypto

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LIBS) -o $(TARGET)

clean:
	rm -f $(TARGET)
