#!/bin/sh

set -x

CURRENT_DIR=`pwd`
BUILD_TYPE=${BUILD_TYPE:-release}
BUILD_BIR=${CURRENT_DIR}/build/${BUILD_TYPE}
BUILD_NO_EXAMPLES=${BUILD_NO_EXAMPLES:-0}
BUILD_NO_TEST=${BUILD_NO_TEST:-0}

if [ ${BUILD_TYPE} != "Release" ] && [ ${BUILD_TYPE} != "Debug" ]
then
    echo "Usage: BUILD_TYPE=Debug/Release"
else
    mkdir -p ${BUILD_BIR}
    cd ${BUILD_BIR}
    cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
          -DCMAKE_BUILD_NO_EXAMPLES=${BUILD_NO_EXAMPLES} \
          -DCMAKE_BUILD_NO_TEST=${BUILD_NO_TEST} \
          ${CURRENT_DIR}
    make
fi
