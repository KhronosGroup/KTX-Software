all: \
    $(SHARED_INTERMEDIATE_DIR)/textoverlay.frag.spv

$(SHARED_INTERMEDIATE_DIR)/textoverlay.frag.spv \
    : \
    tests/loadtests/appfwSDL/VulkanAppSDL/shaders/textoverlay.frag
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)"
	@echo note: "Compiling textoverlay.frag."
	"$(VULKAN_SDK)/bin/glslc" "-fshader-stage=fragment" -o "$(SHARED_INTERMEDIATE_DIR)/textoverlay.frag.spv" "tests/loadtests/appfwSDL/VulkanAppSDL/shaders/textoverlay.frag"
