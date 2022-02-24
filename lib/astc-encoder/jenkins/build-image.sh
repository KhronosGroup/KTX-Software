#!/usr/bin/env bash
ARTIFACTORY_URL=eu-west-1.artifactory.aws.arm.com
DOCKER_REGISTRY=mobile-studio--docker.${ARTIFACTORY_URL}
IMAGE_NAME=astcenc
IMAGE_VERSION=3.1.0

# Check Artifactory credentials are set
if [[ -z "${ARTIFACTORY_CREDENTIALS}" ]]
then
    echo "ARTIFACTORY_CREDENTIALS not set"
    echo "e.g."
    echo "  export ARTIFACTORY_CREDENTIALS=my.name@arm.com:API-KEY"
    exit 1
fi

echo "Preparation"
rm -fr tmp
mkdir -p tmp

echo "Get static analysis tools"
curl --user ${ARTIFACTORY_CREDENTIALS} https://${ARTIFACTORY_URL}/artifactory/mobile-studio.tools/coverity/cov-analysis-linux64-2020.12.sh --output tmp/coverity_install.sh
curl --user ${ARTIFACTORY_CREDENTIALS} https://${ARTIFACTORY_URL}/artifactory/mobile-studio.tools/coverity/license.dat --output tmp/coverity_license.dat

echo "Building image"
docker build -f jenkins/build.Dockerfile \
    -t $IMAGE_NAME:latest \
    -t $IMAGE_NAME:$IMAGE_VERSION \
    -t $DOCKER_REGISTRY/$IMAGE_NAME:latest \
    -t $DOCKER_REGISTRY/$IMAGE_NAME:$IMAGE_VERSION \
    tmp/

echo "Clean up temp files"
rm -rf tmp

if [ "${1}" = "push" ]
then
    echo "Pushing to $DOCKER_REGISTRY"
    docker login -u ${ARTIFACTORY_CREDENTIALS%:*} -p ${ARTIFACTORY_CREDENTIALS#*:} $DOCKER_REGISTRY
    docker push $DOCKER_REGISTRY/$IMAGE_NAME:$IMAGE_VERSION
    # docker push $DOCKER_REGISTRY/$IMAGE_NAME:latest
    echo "Clean up images"
    docker rmi $IMAGE_NAME:latest $IMAGE_NAME:$IMAGE_VERSION $DOCKER_REGISTRY/$IMAGE_NAME:latest $DOCKER_REGISTRY/$IMAGE_NAME:$IMAGE_VERSION
else
    echo "Build complete. To manually push to registry, run:"
    echo "  docker login -u ${ARTIFACTORY_CREDENTIALS%:*} -p ${ARTIFACTORY_CREDENTIALS#*:} $DOCKER_REGISTRY"
    echo "  docker push \"$DOCKER_REGISTRY/$IMAGE_NAME:$IMAGE_VERSION\""
    # echo "  docker push \"$DOCKER_REGISTRY/$IMAGE_NAME:latest\""
fi

echo "Script Completed"
