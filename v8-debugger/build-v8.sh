#!/bin/bash

function LOG {
    echo -e "\x1B[32m$1\x1B[0m"
}

function LOG_ERROR {
    if [ $? -ne 0 ]; then
        echo -e "\x1B[31m$1\x1B[0m"
        exit 1
    fi
}

if [ "$#" != "1" ] ; then
    echo "This script expects one parameter with the target v8 tag version"
    exit 1
fi

LOG "Dowloading depot_tools..."
rm -fr depot_tools 2> /dev/null
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
LOG_ERROR "Error dowloading depot_tools"
export PATH=`pwd`/depot_tools:$PATH

LOG "Downloading v8 source code..."
mkdir v8_$1
cd v8_$1
fetch v8
LOG_ERROR "Error dowloading v8 source code"

# Clone the target tag:
# Node 7.4.0 -> 5.4.500.40
# Node 7.8.0 -> 5.5.372.40
# Electron 1.6.5 -> 5.6.326.50
LOG "Checking out the target v8 tag ($1)..."
cd v8
git checkout tags/$1
LOG_ERROR "Error checking out the target v8 tag ($1)"

LOG "Downloading build dependencies..."
gclient sync
LOG "Error downloading build dependencies"

LOG "Building v8..."
make x64.debug -j4
LOG_ERROR "Error building v8"

# Create a symbolic link to v8_$1/v8 so it is easy to switch v8 versions in the Xcode project
LOG "Creating symbolic link..."
rm -fr v8 2> /dev/null
ln -s "$(pwd)/v8_$1/v8" "$(pwd)/v8"
LOG_ERROR "Error creating symbolic link"

LOG "v8 copiled without errors"
