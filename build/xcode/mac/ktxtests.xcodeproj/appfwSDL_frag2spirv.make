all: \
    $(SHARED_INTERMEDIATE_DIR)/textoverlay.frag.spv

$(SHARED_INTERMEDIATE_DIR)/textoverlay.frag.spv \
    : \
    tests/loadtests/appfwSDL/VulkanAppSDL/shaders/textoverlay.frag
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)"
	@echo note: "Compiling textoverlay.frag."
	"$(VULKAN_SDK)/../MoltenShaderConverter/Tools/MoltenShaderConverter" -gi "tests/loadtests/appfwSDL/VulkanAppSDL/shaders/textoverlay.frag" -so "$(SHARED_INTERMEDIATE_DIR)/textoverlay.frag.spv"
