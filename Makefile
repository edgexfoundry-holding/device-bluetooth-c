
.PHONY: build test clean docker

MICROSERVICES=build/release/device-ble-c/device-ble-c
.PHONY: $(MICROSERVICES)

DOCKERS=docker_device_ble_c
.PHONY: $(DOCKERS)

VERSION=$(shell cat ./VERSION || echo 0.0.0)
GIT_SHA=$(shell git rev-parse HEAD)

build: ${MICROSERVICES}

build/release/device-ble-c/device-ble-c:
	    scripts/build.sh

test:
	    @echo $(MICROSERVICES)


clean:
	    rm -f $(MICROSERVICES)

docker: $(DOCKERS)

docker_device_ble_c:
	    docker build \
	        -f scripts/Dockerfile.alpine-3.9 \
	        --label "git_sha=$(GIT_SHA)" \
	        -t edgexfoundry/docker-device-ble-c:${GIT_SHA} \
	        -t edgexfoundry/docker-device-ble-c:${VERSION}-dev \
            .


