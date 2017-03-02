all: \
    $(INTERMEDIATE_DIR)/cube.frag.spv \
    $(INTERMEDIATE_DIR)/reflect.frag.spv \
    $(INTERMEDIATE_DIR)/skybox.frag.spv \
    $(INTERMEDIATE_DIR)/texture.frag.spv \
    $(INTERMEDIATE_DIR)/instancing.frag.spv

$(INTERMEDIATE_DIR)/cube.frag.spv \
    : \
    tests/loadtests/vkloadtests/shaders/cube/cube.frag
	@mkdir -p "$(INTERMEDIATE_DIR)"
	@echo note: "Compiling cube.frag."
	"$(VULKAN_SDK)/../MoltenShaderConverter/Tools/MoltenShaderConverter" -gi "tests/loadtests/vkloadtests/shaders/cube/cube.frag" -so "$(INTERMEDIATE_DIR)/cube.frag.spv"

$(INTERMEDIATE_DIR)/reflect.frag.spv \
    : \
    tests/loadtests/vkloadtests/shaders/cubemap/reflect.frag
	@mkdir -p "$(INTERMEDIATE_DIR)"
	@echo note: "Compiling reflect.frag."
	"$(VULKAN_SDK)/../MoltenShaderConverter/Tools/MoltenShaderConverter" -gi "tests/loadtests/vkloadtests/shaders/cubemap/reflect.frag" -so "$(INTERMEDIATE_DIR)/reflect.frag.spv"

$(INTERMEDIATE_DIR)/skybox.frag.spv \
    : \
    tests/loadtests/vkloadtests/shaders/cubemap/skybox.frag
	@mkdir -p "$(INTERMEDIATE_DIR)"
	@echo note: "Compiling skybox.frag."
	"$(VULKAN_SDK)/../MoltenShaderConverter/Tools/MoltenShaderConverter" -gi "tests/loadtests/vkloadtests/shaders/cubemap/skybox.frag" -so "$(INTERMEDIATE_DIR)/skybox.frag.spv"

$(INTERMEDIATE_DIR)/texture.frag.spv \
    : \
    tests/loadtests/vkloadtests/shaders/texture/texture.frag
	@mkdir -p "$(INTERMEDIATE_DIR)"
	@echo note: "Compiling texture.frag."
	"$(VULKAN_SDK)/../MoltenShaderConverter/Tools/MoltenShaderConverter" -gi "tests/loadtests/vkloadtests/shaders/texture/texture.frag" -so "$(INTERMEDIATE_DIR)/texture.frag.spv"

$(INTERMEDIATE_DIR)/instancing.frag.spv \
    : \
    tests/loadtests/vkloadtests/shaders/texturearray/instancing.frag
	@mkdir -p "$(INTERMEDIATE_DIR)"
	@echo note: "Compiling instancing.frag."
	"$(VULKAN_SDK)/../MoltenShaderConverter/Tools/MoltenShaderConverter" -gi "tests/loadtests/vkloadtests/shaders/texturearray/instancing.frag" -so "$(INTERMEDIATE_DIR)/instancing.frag.spv"
