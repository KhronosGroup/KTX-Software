all: \
    $(SHARED_INTERMEDIATE_DIR)/textoverlay.vert.spv

$(SHARED_INTERMEDIATE_DIR)/textoverlay.vert.spv \
    : \
    tests/loadtests/appfwSDL/VulkanAppSDL/shaders/textoverlay.vert
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)"
	@echo note: "Compiling textoverlay.vert."
	"$(VULKAN_SDK)/bin/glslc" "-fshader-stage=vertex" -o "$(SHARED_INTERMEDIATE_DIR)/textoverlay.vert.spv" "tests/loadtests/appfwSDL/VulkanAppSDL/shaders/textoverlay.vert"
