all: \
    $(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/textoverlay.vert.spv

$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/textoverlay.vert.spv \
    : \
    tests/loadtests/appfwSDL/VulkanAppSDL/shaders/textoverlay.vert
	@mkdir -p "$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders"
	@echo note: "Compiling textoverlay.vert."
	"$(VULKAN_SDK)/../MoltenShaderConverter/Tools/MoltenShaderConverter" -gi "tests/loadtests/appfwSDL/VulkanAppSDL/shaders/textoverlay.vert" -so "$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/textoverlay.vert.spv"
