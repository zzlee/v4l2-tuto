BUILD_WITH_CUDA:=ON
BUILD_WITH_NPP:=ON
BUILD_WITH_NVBUF:=ON

include ../Rules.mk

APP := 03_qvio-ctl

SRCS := \
	main.cpp \
	$(wildcard $(COMMON_DIR)/*.cpp) \
	kernels.cu

OBJS := $(SRCS:.cpp=.o)
OBJS := $(OBJS:.cu=.o)

.PHONY: all clean

all: $(APP)

clean:
	$(AT)rm -rf $(APP) $(OBJS)

include ../Targets.mk

$(COMMON_DIR)/%.o: $(COMMON_DIR)/%.cpp
	$(AT)$(MAKE) -C $(COMMON_DIR)

$(APP): $(OBJS)
	@echo "Linking: $@"
	$(AT)$(CXX) -o $@ $(OBJS) $(CXXFLAGS) $(LDFLAGS)
