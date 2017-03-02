all: \
    $(INTERMEDIATE_DIR)/textoverlay.frag.spv

$(INTERMEDIATE_DIR)/textoverlay.frag.spv \
    : \
    tests/loadtests/appfwSDL/VulkanAppSDL/shaders/textoverlay.frag
	@mkdir -p "$(INTERMEDIATE_DIR)"
	@echo note: "Compiling textoverlay.frag."
	"$(VULKAN_SDK)/../MoltenShaderConverter/Tools/MoltenShaderConverter" -gi "tests/loadtests/appfwSDL/VulkanAppSDL/shaders/textoverlay.frag" -so "$(INTERMEDIATE_DIR)/textoverlay.frag.spv"
