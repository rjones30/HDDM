#!/bin/bash
#
# openssl_macos_installer.sh - script to install openssl in userspace
#                              on a generic macos host from sources, assuming that
#                              development tools (gnu compilers, linker) and the
#                              curl utility are already present.
#
# author: richard.t.jones at uconn.edu
# version: june 5, 2024

release="https://www.openssl.org/source/openssl-3.2.1.tar.gz"
tarball=$(basename $release)

function usage() {
    echo "Usage: openssl_macos_installer.sh <install_prefix>"
    echo " where <install_prefix>/lib is the intended destination"
    echo " for the openssl libraries."
    exit 1
}
function error_exit() {
    echo "openssl_macos_installer.sh error - $2"
    exit $1
}

if [ $# -ne 1 ]; then
    usage
elif ! which curl >/dev/null 2>/dev/null; then
    error_exit $? "curl command is not available, cannot continue."
else
    install_prefix=$1
fi

export PATH=$install_prefix/bin:$PATH
if ! perl -MCPAN -e update CPAN 2>/dev/null >/dev/null; then
    error_exit $? "perl CPAN is not installed, cannot continue."
fi

curl $release -o $tarball || error_exit $? "unable to GET $release"
tar -zxf $tarball
source=$(echo $tarball | sed 's/.tar.gz$//')
cd $source
./config no-shared --prefix=$install_prefix --openssldir=$install_prefix
make VERBOSE=1
make install

if ! $install_prefix/bin/openssl version; then
    error_exit $? "openssl installation failed"
else
    echo "openssl installation completed successfully!"
fi
