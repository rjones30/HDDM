#!/bin/bash
#
# perl_macos_installer.sh - script to install perl with CPAN in userspace
#                           on a macos host from sources, assuming that
#                           development tools (xcode compilers, linker)
#                           and the curl utility are already present.
#
# author: richard.t.jones at uconn.edu
# version: june 5, 2024

release=https://www.cpan.org/src/5.0/perl-5.32.0.tar.gz
tarball=$(basename $release)

function usage() {
    echo "Usage: perl_macos_installer.sh <install_prefix>"
    echo " where <install_prefix>/bin is the intended destination"
    echo " for the perl executables, and <install_prefix>/lib is"
    echo " the intended installation directory for modules."
    exit 1
}
function error_exit() {
    echo "perl_macos_installer.sh error - $2"
    exit $1
}

if [ $# -ne 1 ]; then
    usage
elif ! which curl >/dev/null 2>/dev/null; then
    error_exit $? "curl command is not available, cannot continue."
elif perl -MCPAN -e update CPAN 2>/dev/null >/dev/null; then
    echo "perl is already installed, CPAN is up to date, no further action needed."
    exit 0
else
    install_prefix=$1
fi

curl $release -o $tarball || error_exit $? "unable to GET $release"
tar -zxf $tarball
source=$(echo $tarball | sed 's/.tar.gz$//')
cd $source
./Configure -des -Dprefix=$install_prefix
make VERBOSE=1
make install

if ! $install_prefix/bin/perl -MCPAN -e update CPAN 2>/dev/null >/dev/null; then
    error_exit $? "perl installation failed"
else
    echo "perl installation completed successfully!"
fi
