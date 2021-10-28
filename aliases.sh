#
# aliases for docker-based platformio
# based on that script in sglahn/platformio-core
#


#
# USAGE: substitute dpio for pio
# e.g.
# dpio run
dpio()
{
    IMAGE_NAME=sglahn/platformio-core:latest
    
    DEVICE=
    if [ -e /dev/ttyUSB0 ]; then
	DEVICE="--device=/dev/ttyUSB0"
    fi
    if [ "$UPLOAD_PORT" ]; then
	DEVICE=$UPLOAD_PORT
    fi
    if [ "$DEVICE" ]; then
	echo "Using upload port $DEVICE"
    fi

    docker run --rm \
	   -v $(pwd)/dotplatformio:/.platformio \
	   -v "$(pwd)":/workspace \
	   -u `id -u $USER`:`id -g $USER` \
	   $DEVICE \
	   $IMAGE_NAME \
	   $@
}

#
# use dpio_bash to run the docker container in interactive mode
# with a bash prompt
dpio_bash()
{
    IMAGE_NAME=sglahn/platformio-core:latest
    
    DEVICE=
    if [ -e /dev/ttyUSB0 ]; then
	DEVICE="--device=/dev/ttyUSB0"
    fi
    if [ "$UPLOAD_PORT" ]; then
	DEVICE=$UPLOAD_PORT
    fi
    if [ "$DEVICE" ]; then
	echo "Using upload port $DEVICE"
    fi

    docker run --rm \
	   -it \
	   -v $(pwd)/dotplatformio:/.platformio \
	   -v "$(pwd)":/workspace \
	   -u `id -u $USER`:`id -g $USER` \
	   $DEVICE \
	   --entrypoint=/bin/bash \
	   $IMAGE_NAME \
	   -i 
}

