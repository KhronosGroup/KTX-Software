all: \
    $(SHARED_INTERMEDIATE_DIR)/cube.vert.spv \
    $(SHARED_INTERMEDIATE_DIR)/reflect.vert.spv \
    $(SHARED_INTERMEDIATE_DIR)/skybox.vert.spv \
    $(SHARED_INTERMEDIATE_DIR)/texture.vert.spv \
    $(SHARED_INTERMEDIATE_DIR)/instancing.vert.spv

$(SHARED_INTERMEDIATE_DIR)/cube.vert.spv \
    : \
    tests/loadtests/vkloadtests/shaders/cube/cube.vert
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)"
	@echo note: "Compiling cube.vert."
	"$(VULKAN_SDK)/../MoltenShaderConverter/Tools/MoltenShaderConverter" -gi "tests/loadtests/vkloadtests/shaders/cube/cube.vert" -so "$(SHARED_INTERMEDIATE_DIR)/cube.vert.spv"

$(SHARED_INTERMEDIATE_DIR)/reflect.vert.spv \
    : \
    tests/loadtests/vkloadtests/shaders/cubemap/reflect.vert
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)"
	@echo note: "Compiling reflect.vert."
	"$(VULKAN_SDK)/../MoltenShaderConverter/Tools/MoltenShaderConverter" -gi "tests/loadtests/vkloadtests/shaders/cubemap/reflect.vert" -so "$(SHARED_INTERMEDIATE_DIR)/reflect.vert.spv"

$(SHARED_INTERMEDIATE_DIR)/skybox.vert.spv \
    : \
    tests/loadtests/vkloadtests/shaders/cubemap/skybox.vert
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)"
	@echo note: "Compiling skybox.vert."
	"$(VULKAN_SDK)/../MoltenShaderConverter/Tools/MoltenShaderConverter" -gi "tests/loadtests/vkloadtests/shaders/cubemap/skybox.vert" -so "$(SHARED_INTERMEDIATE_DIR)/skybox.vert.spv"

$(SHARED_INTERMEDIATE_DIR)/texture.vert.spv \
    : \
    tests/loadtests/vkloadtests/shaders/texture/texture.vert
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)"
	@echo note: "Compiling texture.vert."
	"$(VULKAN_SDK)/../MoltenShaderConverter/Tools/MoltenShaderConverter" -gi "tests/loadtests/vkloadtests/shaders/texture/texture.vert" -so "$(SHARED_INTERMEDIATE_DIR)/texture.vert.spv"

$(SHARED_INTERMEDIATE_DIR)/instancing.vert.spv \
    : \
    tests/loadtests/vkloadtests/shaders/texturearray/instancing.vert
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)"
	@echo note: "Compiling instancing.vert."
	"$(VULKAN_SDK)/../MoltenShaderConverter/Tools/MoltenShaderConverter" -gi "tests/loadtests/vkloadtests/shaders/texturearray/instancing.vert" -so "$(SHARED_INTERMEDIATE_DIR)/instancing.vert.spv"
