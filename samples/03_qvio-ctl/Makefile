include ../Rules.mk

APP := 03_qvio-ctl

SRCS := \
	main.cpp \
	$(wildcard $(COMMON_DIR)/*.cpp)

OBJS := $(SRCS:.cpp=.cpp.o)
OBJS += $(SRCS:.cu=.cu.o)

all: $(APP)

clean:
	$(AT)rm -rf $(APP) $(OBJS)

$(COMMON_DIR)/%.o: $(COMMON_DIR)/%.cpp
	$(AT)$(MAKE) -C $(COMMON_DIR)

%.cpp.o: %.cpp
	@echo "Compiling: $<"
	$(AT)$(CXX) $(CXXFLAGS) -fPIC -c $< -o $@

%.cu.o: %.cu
	@echo "NVCC compiling: $<"
	$(AT)$(NVCC) $(CUFLAGS) -c $< -o $@

$(APP): $(OBJS)
	@echo "Linking: $@"
	$(AT)$(CXX) -o $@ $(OBJS) $(CXXFLAGS) $(LDFLAGS)
