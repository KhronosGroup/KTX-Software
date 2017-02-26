all: \
    $(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/cube.frag.spv \
    $(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/reflect.frag.spv \
    $(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/skybox.frag.spv \
    $(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/texture.frag.spv \
    $(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/instancing.frag.spv

$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/cube.frag.spv \
    : \
    tests/loadtests/vkloadtests/shaders/cube/cube.frag
	@mkdir -p "$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders"
	@echo note: "Compiling cube.frag."
	"$(VULKAN_SDK)/../MoltenShaderConverter/Tools/MoltenShaderConverter" -gi "tests/loadtests/vkloadtests/shaders/cube/cube.frag" -so "$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/cube.frag.spv"

$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/reflect.frag.spv \
    : \
    tests/loadtests/vkloadtests/shaders/cubemap/reflect.frag
	@mkdir -p "$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders"
	@echo note: "Compiling reflect.frag."
	"$(VULKAN_SDK)/../MoltenShaderConverter/Tools/MoltenShaderConverter" -gi "tests/loadtests/vkloadtests/shaders/cubemap/reflect.frag" -so "$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/reflect.frag.spv"

$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/skybox.frag.spv \
    : \
    tests/loadtests/vkloadtests/shaders/cubemap/skybox.frag
	@mkdir -p "$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders"
	@echo note: "Compiling skybox.frag."
	"$(VULKAN_SDK)/../MoltenShaderConverter/Tools/MoltenShaderConverter" -gi "tests/loadtests/vkloadtests/shaders/cubemap/skybox.frag" -so "$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/skybox.frag.spv"

$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/texture.frag.spv \
    : \
    tests/loadtests/vkloadtests/shaders/texture/texture.frag
	@mkdir -p "$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders"
	@echo note: "Compiling texture.frag."
	"$(VULKAN_SDK)/../MoltenShaderConverter/Tools/MoltenShaderConverter" -gi "tests/loadtests/vkloadtests/shaders/texture/texture.frag" -so "$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/texture.frag.spv"

$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/instancing.frag.spv \
    : \
    tests/loadtests/vkloadtests/shaders/texturearray/instancing.frag
	@mkdir -p "$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders"
	@echo note: "Compiling instancing.frag."
	"$(VULKAN_SDK)/../MoltenShaderConverter/Tools/MoltenShaderConverter" -gi "tests/loadtests/vkloadtests/shaders/texturearray/instancing.frag" -so "$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/instancing.frag.spv"
