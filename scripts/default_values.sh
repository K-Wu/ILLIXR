#!/bin/bash

#--- Script defining values to be used by accompanying install scripts ---#


### Default imported variables setup ###

if [ -z "${opt_dir}" ]; then
    export opt_dir="/opt/ILLIXR"
fi

if [ -z "${prefix_dir}" ]; then
    export prefix_dir="/usr/local"
fi

if [ -z "${illixr_nproc}" ]; then
    export illixr_nproc="1"
fi

if [ -z "${build_type}" ]; then
    export build_type="Release"
fi


### For use with 'scripts/install_apt_deps.sh' ###

if [ -z "${use_realsense}" ]; then
    export use_realsense="no"
fi

if [ -z "${use_docker}" ]; then
    export use_docker="no"
fi

if [ -z "${use_cuda}" ]; then
    export use_cuda="no"
fi
