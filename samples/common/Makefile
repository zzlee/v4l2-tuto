include ../Rules.mk

SRCS:= $(wildcard *.cpp)

OBJS:= $(SRCS:.cpp=.o)

all: $(OBJS)

%.o: %.cpp
	@echo "Compiling: $<"
	$(AT)$(CXX) $(CXXFLAGS) -fPIC -c $<

clean:
	$(AT)rm -rf $(OBJS)
