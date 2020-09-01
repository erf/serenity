#!/bin/bash ../.port_include.sh
port=vis
version=git
workdir=vis-master
useconfigure="true"
#files="https://github.com/martanne/vis/releases/download/v${version}/vis-${version}.tar.gz vis-${version}.tar.gz"
files="https://github.com/martanne/vis/archive/master.tar.gz vis-master.tar.gz"

configopts="--enable-curses=no"
depends="libtermkey"
#auth_type="sig"
#auth_opts="--keyring ./gnu-keyring.gpg vis-${version}.tar.gz.sig"

#export CPPFLAGS=-I${SERENITY_ROOT}/Build/Root/usr/local/include/ncurses
#export PKG_CONFIG_PATH=${SERENITY_ROOT}/Build/Root/usr/local/lib/pkgconfig

