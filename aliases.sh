#
# do "platformio run" from within docker
alias dp-run="docker run --rm  -v $(pwd)/dotplatformio:/.platformio --mount type=bind,source="$(pwd)",target=/workspace  -u `id -u $USER`:`id -g $USER` sglahn/platformio-core:latest run"

#
# get bash prompt from inside docker
alias dp_bash="docker run --rm  -it -v $(pwd)/dotplatformio:/.platformio --mount type=bind,source="$(pwd)",target=/workspace  -u `id -u $USER`:`id -g $USER` --entrypoint=/bin/bash sglahn/platformio-core:latest -i"

