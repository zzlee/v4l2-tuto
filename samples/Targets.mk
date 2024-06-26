%.o: %.cpp
	@echo "CXX: $< ==> $@"
	$(AT)$(CXX) $(CXXFLAGS) -fPIC -c $< -o $@

%.o: %.cu
	@echo "NVCC: $< ==> $@"
	$(AT)$(NVCC) $(CUFLAGS) -c $< -o $@
