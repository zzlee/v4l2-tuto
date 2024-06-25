include ../Rules.mk

SRCS:= $(wildcard *.cpp)

OBJS:= $(SRCS:.cpp=.cpp.o)

all: $(OBJS)

%.cpp.o: %.cpp
	@echo "Compiling: $<"
	$(AT)$(CXX) $(CXXFLAGS) -fPIC -c $< -o $@

clean:
	$(AT)rm -rf $(OBJS)
