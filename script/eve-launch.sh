#!/bin/sh

# Arguments : -p <webkit_path> -m <webkit_mode> 
usage()
{
  cat << EOF
usage: $0 options
 This script launch Eve.
 Proxy setting can be set with the http_poxy environment variable.
 OPTIONS:
    -h      Show this message
    -d      Run eve with gdb
    -g      Run eve with WEBKIT_DEBUG=<module,...>
    -p      Set webkit source base directory
    -b      Set webkit build directory (relative to webkit source base directory)
    -m      Set webkit build mode (Debug|Release)
    -n      Unset http_proxy and https_proxy

    $0 -p <url>
EOF
}

GDB=
WEBKIT_LOG=
DEFAULT_PAGE=
WEBKIT_PATH=
WEBKIT_MODE=
WEBKIT_BUILD_DIR=

while getopts “p:m:b:g:dthn” OPTION
do
      case $OPTION in
            h)
                  usage
                  exit 1
                  ;;
            d)
                  GDB="gdb --args"
                  ;;
            g)
                  WEBKIT_LOG=$OPTARG
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
            n)
                  unset http_proxy
	 	              unset https_proxy
                  ;;
            ?)
                  usage
                  exit
                  ;;
     esac
done

shift $(( OPTIND-1 ))

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

if [ $# -eq 0 ]
then
    DEFAULT_PAGE="file://$PWD/index.html"
    echo Set default startup page $DEFAULT_PAGE
fi

WEBKIT_DIR=$WEBKIT_PATH/$WEBKIT_BUILD_DIR/$WEBKIT_MODE

if [ -z $http_proxy ]
then
  EINA_LOG_LEVEL= \
  http_proxy= https_proxy= LD_LIBRARY_PATH=/usr/local/lib:$WEBKIT_DIR/lib \
  WEBKIT_DEBUG=$WEBKIT_LOG $GDB $WEBKIT_DIR/../Dependencies/Root/bin/eve $DEFAULT_PAGE $*
else
  EINA_LOG_LEVEL= \
  LD_LIBRARY_PATH=/usr/local/lib:$WEBKIT_DIR/lib \
  WEBKIT_DEBUG=$WEBKIT_LOG $GDB $WEBKIT_DIR/../Dependencies/Root/bin/eve $DEFAULT_PAGE $*
fi
