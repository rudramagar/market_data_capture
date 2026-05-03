CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -I include
LDFLAGS  =

TARGET   = xnet-bmd-capture
BUILDDIR = build

SRCS     = src/config_parser.cpp \
           src/socket.cpp \
           src/capture.cpp \
           src/main.cpp

OBJS     = $(patsubst src/%.cpp,$(BUILDDIR)/%.o,$(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

$(BUILDDIR)/%.o: src/%.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR) $(TARGET)

.PHONY: all clean
