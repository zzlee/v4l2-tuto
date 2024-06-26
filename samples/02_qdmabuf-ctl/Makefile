include ../Rules.mk

APP := 02_qdmabuf-ctl

SRCS := \
	main.cpp \
	$(wildcard $(COMMON_DIR)/*.cpp)

OBJS := $(SRCS:.cpp=.o)
OBJS := $(OBJS:.cu=.o)

all: $(APP)

clean:
	$(AT)rm -rf $(APP) $(OBJS)

include ../Targets.mk

$(COMMON_DIR)/%.o: $(COMMON_DIR)/%.cpp
	$(AT)$(MAKE) -C $(COMMON_DIR)

$(APP): $(OBJS)
	@echo "Linking: $@"
	$(AT)$(CXX) -o $@ $(OBJS) $(CXXFLAGS) $(LDFLAGS)
