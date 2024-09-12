sudo docker build --target ran-build --tag ran-build:latest --file docker/Dockerfile.build.ubuntu20 --build-arg "BUILD_OPTION=--sanitize" .
