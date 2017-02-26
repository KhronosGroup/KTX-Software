all: \
    $(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/textoverlay.frag.spv

$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/textoverlay.frag.spv \
    : \
    tests/loadtests/appfwSDL/VulkanAppSDL/shaders/textoverlay.frag
	@mkdir -p "$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders"
	@echo note: "Compiling textoverlay.frag."
	"$(VULKAN_SDK)/../MoltenShaderConverter/Tools/MoltenShaderConverter" -gi "tests/loadtests/appfwSDL/VulkanAppSDL/shaders/textoverlay.frag" -so "$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/textoverlay.frag.spv"
