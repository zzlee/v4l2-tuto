$(COMMON_DIR)/%.cpp.o: $(COMMON_DIR)/%.cpp
	$(AT)$(MAKE) -C $(COMMON_DIR)

%.cpp.o: %.cpp
	@echo "CXX: $< ==> $@"
	$(AT)$(CXX) $(CXXFLAGS) -fPIC -c $< -o $@

%.cu.o: %.cu
	@echo "NVCC: $< ==> $@"
	$(AT)$(NVCC) $(CUFLAGS) -c $< -o $@
