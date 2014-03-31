Demo Content 
============

The demo web application allows controlling local UPnP services that store or render media content.
Service discovery is provided through the W3C [Network Service Discovery](http://www.w3.org/TR/discovery-api/) (NSD) API.
Service access is done using the XMLHttpRequest API.

The web application displays gathered data using four panels: 

* The <i>Browse</i> panel presents the list of <b>ContentDirectory</b> services discovered.
* The <i>Info</i> panel displays the result of the BrowseMetadata API of ContentDirectory.
* The <i>Player</i> panel exposes <b>AVTransport</b> and <b>RenderingControl</b> services.
* The <i>Settings</i> panel is not yet implemented because UPnP devices do not provide information specified in the UPnP rendering control specification: brightness, contrast, RGB Gain, keystone, loudness...

This web application may interact with any UPnP media service and was tested with XBMC, MediaTomb and Windows Media Player.
The following subtitled videos on YouTube show different aspects of NSD usage in the demo:

* [Normal use of the NSD API](http://youtu.be/QSczL-kqDpU)
* [Use of UPnP events to synchronize service and application](http://youtu.be/hmgqnkDhCgo)
* [Service refresh to react on services start/stop](http://youtu.be/Mi5QMO5mfc4)
* [User granting persistent storage to improve the user experience](http://youtu.be/kIyuO3C_DMA)

 
Building the Demo
=================

The Network Service Discovery API is mandatory to properly run this web application.
Its implementation is based on a fork of WebKit implementing the NSD API (available at https://github.com/canonresearchfrance/webkit).
The Eve EFL based browser implements the UI user agent part of the NSD API (integrated as a subtree of this repository, also available at https://github.com/canonresearchfrance/eve).

The build procedure is as follows:

1. Clone this repository on a local folder (called $FOLDER)
2. Retrieve the WebKit NSD implementation zip archive at [github](https://github.com/canonresearchfrance/webkit/archive/webkit-nsd.zip)
3. Unzip its content into $FOLDER/WebKit-NSD
   * Please use WebKit-NSD name as this is the default folder where the eve-build.sh script wil try to find WebKit implementation.
   * Inside WebKit-NSD, you should directly find WebKit source code and not any intermediary folder (it should contain several folders such as LayoutTests, Tools, Source...)
4. You are now ready to build WebKit and Eve browser by opening a terminal, changing the directory path to $FOLDER and typing:
   > <code>./script/eve-build.sh</code>

   Note the following points:
   * Building WebKit can be quite long.
   * Building WebKit requires web access to retrieve all libraries needed to build WebKit. If running behind a firewall, please set properly the <code>http\_proxy</code> and <code>https\_proxy</code> environment variables.
   * It may be needed to install system dependencies prior building WebKit:
     > <code>./WebKit-NSD/Tools/efl/install-dependencies</code>

You can also build WebKit and EVE on your own (see [webkit documentation](http://trac.webkit.org/wiki/EFLWebKit)).
First, please take care of WebKit EFL library dependencies:

> <code>./WebKit-NSD/Tools/Scripts/update-webkitefl-libs</code>

WebKitEFL can be built by running the following command:

> <code>./WebKit-NSD/Tools/Scripts/build-webkit --efl</code>

To build the EVE browser, please use autogen and make tools. 

The build process has been successfully tested of Fedora 20 and Ubuntu 13.04.

Running the Demo
================

Run the application with the following command:

> <code>./script/eve-launch.sh</code>
 
EVE will create in your $HOME directory the following configuration files:

* $HOME/.config/eve/config.eet contains the <i>Enable auto authorize policy</i> preference.
* $HOME/.config/eve/services.eet contains the list of <i>Services registered</i> in <b>Network Discovery preferences</b>.
 
 
Contributors
* Ludovic Le Goff
* Youenn Fablet
