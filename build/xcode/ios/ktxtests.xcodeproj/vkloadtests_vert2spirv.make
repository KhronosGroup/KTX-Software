all: \
    $(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/cube.vert.spv \
    $(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/reflect.vert.spv \
    $(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/skybox.vert.spv \
    $(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/texture.vert.spv \
    $(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/instancing.vert.spv

$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/cube.vert.spv \
    : \
    tests/loadtests/vkloadtests/shaders/cube/cube.vert
	@mkdir -p "$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders"
	@echo note: "Compiling cube.vert."
	"$(VULKAN_SDK)/../MoltenShaderConverter/Tools/MoltenShaderConverter" -gi "tests/loadtests/vkloadtests/shaders/cube/cube.vert" -so "$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/cube.vert.spv"

$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/reflect.vert.spv \
    : \
    tests/loadtests/vkloadtests/shaders/cubemap/reflect.vert
	@mkdir -p "$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders"
	@echo note: "Compiling reflect.vert."
	"$(VULKAN_SDK)/../MoltenShaderConverter/Tools/MoltenShaderConverter" -gi "tests/loadtests/vkloadtests/shaders/cubemap/reflect.vert" -so "$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/reflect.vert.spv"

$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/skybox.vert.spv \
    : \
    tests/loadtests/vkloadtests/shaders/cubemap/skybox.vert
	@mkdir -p "$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders"
	@echo note: "Compiling skybox.vert."
	"$(VULKAN_SDK)/../MoltenShaderConverter/Tools/MoltenShaderConverter" -gi "tests/loadtests/vkloadtests/shaders/cubemap/skybox.vert" -so "$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/skybox.vert.spv"

$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/texture.vert.spv \
    : \
    tests/loadtests/vkloadtests/shaders/texture/texture.vert
	@mkdir -p "$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders"
	@echo note: "Compiling texture.vert."
	"$(VULKAN_SDK)/../MoltenShaderConverter/Tools/MoltenShaderConverter" -gi "tests/loadtests/vkloadtests/shaders/texture/texture.vert" -so "$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/texture.vert.spv"

$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/instancing.vert.spv \
    : \
    tests/loadtests/vkloadtests/shaders/texturearray/instancing.vert
	@mkdir -p "$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders"
	@echo note: "Compiling instancing.vert."
	"$(VULKAN_SDK)/../MoltenShaderConverter/Tools/MoltenShaderConverter" -gi "tests/loadtests/vkloadtests/shaders/texturearray/instancing.vert" -so "$(BUILT_PRODUCTS_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/shaders/instancing.vert.spv"
