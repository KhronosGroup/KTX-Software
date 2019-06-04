all: \
    $(SHARED_INTERMEDIATE_DIR)/cube.vert.spv \
    $(SHARED_INTERMEDIATE_DIR)/reflect.vert.spv \
    $(SHARED_INTERMEDIATE_DIR)/skybox.vert.spv \
    $(SHARED_INTERMEDIATE_DIR)/texture.vert.spv \
    $(SHARED_INTERMEDIATE_DIR)/instancing.vert.spv \
    $(SHARED_INTERMEDIATE_DIR)/instancinglod.vert.spv

$(SHARED_INTERMEDIATE_DIR)/cube.vert.spv \
    : \
    tests/loadtests/vkloadtests/shaders/cube/cube.vert
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)"
	@echo note: "Compiling cube.vert."
	"$(VULKAN_SDK)/bin/glslc" "-fshader-stage=vertex" -o "$(SHARED_INTERMEDIATE_DIR)/cube.vert.spv" "tests/loadtests/vkloadtests/shaders/cube/cube.vert"

$(SHARED_INTERMEDIATE_DIR)/reflect.vert.spv \
    : \
    tests/loadtests/vkloadtests/shaders/cubemap/reflect.vert
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)"
	@echo note: "Compiling reflect.vert."
	"$(VULKAN_SDK)/bin/glslc" "-fshader-stage=vertex" -o "$(SHARED_INTERMEDIATE_DIR)/reflect.vert.spv" "tests/loadtests/vkloadtests/shaders/cubemap/reflect.vert"

$(SHARED_INTERMEDIATE_DIR)/skybox.vert.spv \
    : \
    tests/loadtests/vkloadtests/shaders/cubemap/skybox.vert
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)"
	@echo note: "Compiling skybox.vert."
	"$(VULKAN_SDK)/bin/glslc" "-fshader-stage=vertex" -o "$(SHARED_INTERMEDIATE_DIR)/skybox.vert.spv" "tests/loadtests/vkloadtests/shaders/cubemap/skybox.vert"

$(SHARED_INTERMEDIATE_DIR)/texture.vert.spv \
    : \
    tests/loadtests/vkloadtests/shaders/texture/texture.vert
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)"
	@echo note: "Compiling texture.vert."
	"$(VULKAN_SDK)/bin/glslc" "-fshader-stage=vertex" -o "$(SHARED_INTERMEDIATE_DIR)/texture.vert.spv" "tests/loadtests/vkloadtests/shaders/texture/texture.vert"

$(SHARED_INTERMEDIATE_DIR)/instancing.vert.spv \
    : \
    tests/loadtests/vkloadtests/shaders/texturearray/instancing.vert
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)"
	@echo note: "Compiling instancing.vert."
	"$(VULKAN_SDK)/bin/glslc" "-fshader-stage=vertex" -o "$(SHARED_INTERMEDIATE_DIR)/instancing.vert.spv" "tests/loadtests/vkloadtests/shaders/texturearray/instancing.vert"

$(SHARED_INTERMEDIATE_DIR)/instancinglod.vert.spv \
    : \
    tests/loadtests/vkloadtests/shaders/texturemipmap/instancinglod.vert
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)"
	@echo note: "Compiling instancinglod.vert."
	"$(VULKAN_SDK)/bin/glslc" "-fshader-stage=vertex" -o "$(SHARED_INTERMEDIATE_DIR)/instancinglod.vert.spv" "tests/loadtests/vkloadtests/shaders/texturemipmap/instancinglod.vert"
