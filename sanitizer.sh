sudo docker build --target ran-build --tag ran-build:latest --file docker/Dockerfile.build.ubuntu22 --build-arg "BUILD_OPTION=--sanitize" .
