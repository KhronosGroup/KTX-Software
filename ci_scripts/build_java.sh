echo "Build Java bindings for KTX-Software"
echo "LIBKTX_BINARY_DIR " $LIBKTX_BINARY_DIR

build_libktx_java_dir=interface/java_binding

pushd $build_libktx_java_dir
mvn package
popd
