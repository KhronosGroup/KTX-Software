import? 'uni-python.just'

set allow-duplicate-recipes := true
set allow-duplicate-variables := true

_default:
    @just --list --unsorted

docker_registry := env_var_or_default('DOCKER_REGISTRY', '209479306031.dkr.ecr.us-east-2.amazonaws.com')
docker_registry_namespace := env_var_or_default('DOCKER_REGISTRY_NAMESPACE', 'asi-inc')
project_name := env_var_or_default('PROJECT_NAME', 'uni-geodata')
image_tag := env_var_or_default('IMAGE_TAG', `git rev-parse HEAD`)
build_number_tag := env_var_or_default('BUILD_NUMBER_TAG', '')

export COMPOSE_PROJECT_NAME := project_name

# Fetch latest python justfile from uni-justfile github repository.
fetch:
    curl -H "Authorization: token ${GITHUB_TOKEN}" \
         -H "Accept: application/vnd.github.v3.raw" \
         https://api.github.com/repos/uni-intelligence/uni-justfile/contents/uni-python.just \
         -o uni-python.just

# Targets above this line are part of the conventions we use in our CI pipelines.
# Do not modify them unless you are sure of what you're doing.
# You may add new targets below this comment. Doing so allows you to override the default target definitions from https://github.com/uni-intelligence/uni-justfile

#################
# OVERRIDES     #
#################
