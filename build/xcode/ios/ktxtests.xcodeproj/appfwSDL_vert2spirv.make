all: \
    $(SHARED_INTERMEDIATE_DIR)/textoverlay.vert.spv

$(SHARED_INTERMEDIATE_DIR)/textoverlay.vert.spv \
    : \
    tests/loadtests/appfwSDL/VulkanAppSDL/shaders/textoverlay.vert
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)"
	@echo note: "Compiling textoverlay.vert."
	"$(VULKAN_SDK)/../MoltenShaderConverter/Tools/MoltenShaderConverter" -gi "tests/loadtests/appfwSDL/VulkanAppSDL/shaders/textoverlay.vert" -so "$(SHARED_INTERMEDIATE_DIR)/textoverlay.vert.spv"
