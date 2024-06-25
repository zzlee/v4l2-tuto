include ../Rules.mk

APP := 02_qdmabuf-ctl

SRCS := \
	main.cpp \
	$(wildcard $(COMMON_DIR)/*.cpp)

OBJS := $(SRCS:.cpp.o=.o)

all: $(APP)

clean:
	$(AT)rm -rf $(APP) $(OBJS)

include ../Targets.mk

$(APP): $(OBJS)
	@echo "Linking: $@"
	$(AT)$(CXX) -o $@ $(OBJS) $(CXXFLAGS) $(LDFLAGS)
