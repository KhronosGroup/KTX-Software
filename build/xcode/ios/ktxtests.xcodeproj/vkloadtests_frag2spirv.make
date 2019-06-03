all: \
    $(SHARED_INTERMEDIATE_DIR)/cube.frag.spv \
    $(SHARED_INTERMEDIATE_DIR)/reflect.frag.spv \
    $(SHARED_INTERMEDIATE_DIR)/skybox.frag.spv \
    $(SHARED_INTERMEDIATE_DIR)/texture.frag.spv \
    $(SHARED_INTERMEDIATE_DIR)/instancing.frag.spv \
    $(SHARED_INTERMEDIATE_DIR)/instancinglod.frag.spv

$(SHARED_INTERMEDIATE_DIR)/cube.frag.spv \
    : \
    tests/loadtests/vkloadtests/shaders/cube/cube.frag
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)"
	@echo note: "Compiling cube.frag."
	"$(VULKAN_SDK)/bin/glslc" "-fshader-stage=fragment" -o "$(SHARED_INTERMEDIATE_DIR)/cube.frag.spv" "tests/loadtests/vkloadtests/shaders/cube/cube.frag"

$(SHARED_INTERMEDIATE_DIR)/reflect.frag.spv \
    : \
    tests/loadtests/vkloadtests/shaders/cubemap/reflect.frag
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)"
	@echo note: "Compiling reflect.frag."
	"$(VULKAN_SDK)/bin/glslc" "-fshader-stage=fragment" -o "$(SHARED_INTERMEDIATE_DIR)/reflect.frag.spv" "tests/loadtests/vkloadtests/shaders/cubemap/reflect.frag"

$(SHARED_INTERMEDIATE_DIR)/skybox.frag.spv \
    : \
    tests/loadtests/vkloadtests/shaders/cubemap/skybox.frag
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)"
	@echo note: "Compiling skybox.frag."
	"$(VULKAN_SDK)/bin/glslc" "-fshader-stage=fragment" -o "$(SHARED_INTERMEDIATE_DIR)/skybox.frag.spv" "tests/loadtests/vkloadtests/shaders/cubemap/skybox.frag"

$(SHARED_INTERMEDIATE_DIR)/texture.frag.spv \
    : \
    tests/loadtests/vkloadtests/shaders/texture/texture.frag
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)"
	@echo note: "Compiling texture.frag."
	"$(VULKAN_SDK)/bin/glslc" "-fshader-stage=fragment" -o "$(SHARED_INTERMEDIATE_DIR)/texture.frag.spv" "tests/loadtests/vkloadtests/shaders/texture/texture.frag"

$(SHARED_INTERMEDIATE_DIR)/instancing.frag.spv \
    : \
    tests/loadtests/vkloadtests/shaders/texturearray/instancing.frag
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)"
	@echo note: "Compiling instancing.frag."
	"$(VULKAN_SDK)/bin/glslc" "-fshader-stage=fragment" -o "$(SHARED_INTERMEDIATE_DIR)/instancing.frag.spv" "tests/loadtests/vkloadtests/shaders/texturearray/instancing.frag"

$(SHARED_INTERMEDIATE_DIR)/instancinglod.frag.spv \
    : \
    tests/loadtests/vkloadtests/shaders/texturemipmap/instancinglod.frag
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)"
	@echo note: "Compiling instancinglod.frag."
	"$(VULKAN_SDK)/bin/glslc" "-fshader-stage=fragment" -o "$(SHARED_INTERMEDIATE_DIR)/instancinglod.frag.spv" "tests/loadtests/vkloadtests/shaders/texturemipmap/instancinglod.frag"
