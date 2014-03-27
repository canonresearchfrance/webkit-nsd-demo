#!/bin/bash

# Arguments : -p <webkit_path> -m <webkit_mode> 
usage()
{
  cat << EOF
usage: $0 options
 This script setup and build Eve.
 OPTIONS:
    -h      Show this message
    -p      Set webkit source base directory
    -b      Set webkit build directory (relative to webkit source base directory)
    -m      Set webkit mode (Debug|Release)

    $0 -p $HOME/WebKit -b WebKitBuild.efl -m Debug
EOF
}

PWD=`pwd`
WEBKIT_PATH=
WEBKIT_BUILD_DIR=
WEBKIT_MODE=
while getopts “hp:b:m:” OPTION
do
      case $OPTION in
            h)
                  usage
                  exit 1
                  ;;
            p)
                  WEBKIT_PATH=$OPTARG
                  ;;
            b)
                  WEBKIT_BUILD_DIR=$OPTARG
                  ;;
            m)
                  WEBKIT_MODE=$OPTARG
                  ;;
            ?)
                  usage
                  exit
                  ;;
     esac
done
if [ -z $WEBKIT_PATH ]
then    
    echo "\$WEBKIT_PATH is not set (default $PWD/WebKit-NSD)"
    WEBKIT_PATH=$PWD/WebKit-NSD
fi
if [ -z $WEBKIT_BUILD_DIR ]
then    
    echo "\$WEBKIT_BUILD_DIR is not set (default WebKitBuild)"
    WEBKIT_BUILD_DIR=WebKitBuild
fi
if [ -z $WEBKIT_MODE ]
then    
    echo "\$WEBKIT_MODE is not set (default Release)"
    WEBKIT_MODE=Release
fi

ARCH=
if [ `uname -i` = "x86_64" ]
then
    ARCH=64
fi

BASE_DEP_ROOT_DIR=$WEBKIT_PATH/$WEBKIT_BUILD_DIR/Dependencies/Root

if [ ! -d $BASE_DEP_ROOT_DIR ]
then
    echo "Build WebKit Dependencies"
    pushd $WEBKIT_PATH
    ./Tools/Scripts/update-webkitefl-libs
    popd
fi

if [ ! -d $WEBKIT_PATH/$WEBKIT_BUILD_DIR/Release ]
then
    echo "Build WebKit"
    pushd $WEBKIT_PATH
    ./Tools/Scripts/build-webkit --efl --no-svg --page-visibility-api
    popd    
fi

pushd eve

PKG_CONFIG_PATH=$BASE_DEP_ROOT_DIR/lib$ARCH/pkgconfig \
EWEBKIT_CFLAGS="-I$WEBKIT_PATH/Source/WebKit/efl/ewk \
               -I$BASE_DEP_ROOT_DIR/include/cairo" \
EWEBKIT_LIBS="-L$WEBKIT_PATH/$WEBKIT_BUILD_DIR/$WEBKIT_MODE/lib -lewebkit -lcairo" \
XML_CFLAGS="-I$BASE_DEP_ROOT_DIR/include/libxml2" \
XML_LIBS="-L$BASE_DEP_ROOT_DIR/lib$ARCH -lxml2" \
SOUP_CFLAGS="-I$BASE_DEP_ROOT_DIR/include/libsoup-2.4" \
SOUP_LIBS="-L$BASE_DEP_ROOT_DIR/lib$ARCH -lsoup-2.4" \
GLIB_OBJECT_CFLAGS="-I$BASE_DEP_ROOT_DIR/include/glib-2.0" \
GLIB_OBJECT_LIBS="-L$BASE_DEP_ROOT_DIR/lib$ARCH -lgobject-2.0" \
LDFLAGS="-Wl,-rpath $BASE_DEP_ROOT_DIR/lib$ARCH -Wl,-rpath $PWD/src/bin" \
./autogen.sh --prefix=$BASE_DEP_ROOT_DIR \
             --datadir=$BASE_DEP_ROOT_DIR/data/themes \
             --with-ewebkit-datadir=$WEBKIT_PATH/$WEBKIT_BUILD_DIR/$WEBKIT_MODE/WebCore/platform/efl
             
make clean install
if [ ! -f $WEBKIT_PATH/$WEBKIT_BUILD_DIR/$WEBKIT_MODE/WebCore/platform/efl/themes ]
then
    ln -s $WEBKIT_PATH/$WEBKIT_BUILD_DIR/$WEBKIT_MODE/WebCore/platform/efl/DefaultTheme \
        $WEBKIT_PATH/$WEBKIT_BUILD_DIR/$WEBKIT_MODE/WebCore/platform/efl/themes
fi

popd
