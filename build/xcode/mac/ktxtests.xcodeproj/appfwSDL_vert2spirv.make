all: \
    $(INTERMEDIATE_DIR)/textoverlay.vert.spv

$(INTERMEDIATE_DIR)/textoverlay.vert.spv \
    : \
    tests/loadtests/appfwSDL/VulkanAppSDL/shaders/textoverlay.vert
	@mkdir -p "$(INTERMEDIATE_DIR)"
	@echo note: "Compiling textoverlay.vert."
	"$(VULKAN_SDK)/../MoltenShaderConverter/Tools/MoltenShaderConverter" -gi "tests/loadtests/appfwSDL/VulkanAppSDL/shaders/textoverlay.vert" -so "$(INTERMEDIATE_DIR)/textoverlay.vert.spv"
