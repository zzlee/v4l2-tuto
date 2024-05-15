include ../Rules.mk

APP := 02_qdmabuf-ctl

SRCS := \
	main.cpp \
	$(wildcard $(COMMON_DIR)/*.cpp)

OBJS := $(SRCS:.cpp=.o)

all: $(APP)

clean:
	$(AT)rm -rf $(APP) $(OBJS)

$(COMMON_DIR)/%.o: $(COMMON_DIR)/%.cpp
	$(AT)$(MAKE) -C $(COMMON_DIR)

%.o: %.cpp
	@echo "Compiling: $<"
	$(AT)$(CXX) $(CXXFLAGS) -fPIC -c $<

$(APP): $(OBJS)
	@echo "Linking: $@"
	$(AT)$(CXX) -o $@ $(OBJS) $(CXXFLAGS) $(LDFLAGS)
