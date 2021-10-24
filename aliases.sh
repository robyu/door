#
# do "platformio run" from within docker
alias dp_run="docker run --rm  -v $(pwd)/dotplatformio:/.platformio --mount type=bind,source="$(pwd)",target=/workspace  -u `id -u $USER`:`id -g $USER` sglahn/platformio-core:latest run"

#
# get bash prompt from inside docker
alias dp_bash="docker run --rm  -it -v $(pwd)/dotplatformio:/.platformio --mount type=bind,source="$(pwd)",target=/workspace  -u `id -u $USER`:`id -g $USER` --entrypoint=/bin/bash sglahn/platformio-core:latest -i"


#
# "dp" is a general docker platformio-core replacement for "pio" or "platformio"
dp()
{
    params=()
    if [ -n ${1} ]; then
	echo "1: ${1}"
	params+=( ${1} )
    fi
    if [ -n ${2} ]; then
	echo "2: ${2}"
	params+=( ${2} )
    fi
    if [ -n ${3} ]; then
	echo "3: ${3}"
	params+=( ${3} )
    fi
    echo "params=${params[@]}"
    
    docker run --rm  -v $(pwd)/dotplatformio:/.platformio \
	   --mount type=bind,source="$(pwd)",target=/workspace \
	   -u `id -u $USER`:`id -g $USER` sglahn/platformio-core:latest "${params[@]}"
}

