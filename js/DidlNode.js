/*
 * Copyright (C) Canon Inc. 2014
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted, provided that the following conditions
 * are required to be met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Canon Inc. nor the names of 
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CANON INC. AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL CANON INC. AND ITS CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

var DidlNode = function(parentNode, contentDirectoryIndex, xDidl, callback) {

    var self = this;

    this.contentHandler = new DefaultHandler2();

    this.parentNode = parentNode;
    this.index = contentDirectoryIndex;
    this.url = undefined;
    this.rawDidl = xDidl;
    this.callback = callback;

    this.nodes = new Array();
    
    //console.log("xDidl" + xDidl);

    this.contentHandler.startElement = function(namespaceURI, localName, qName, atts) {

        var e = self.currentEntry;
        var hdl = self.contentHandler;

        if ( (localName == "container") || (localName == "item") )  {
            for (var i = 0 ; i < atts.getLength() ; i++)
                if (atts.getLocalName(i) == "id") {
                    self.currentEntry = new Entry(self.index, atts.getValue(i), self.parentNode, self.rawDidl);
                    self.nodes[self.nodes.length] = self.currentEntry;
                    if (self.callback)
                        self.callback(atts.getValue(i), self.currentEntry);
                } 
        } else if ( (localName == "title") ||
                    (localName == "genre") || 
                    (localName == "artist") ||
                    (localName == "author") ||
                    (localName == "album") ||
                    (localName == "originalTrackNumber") ||
                    (localName == "date") ||
                    (localName == "actor") ||
                    (localName == "albumArtURI") ) {
            e.setProp(hdl, localName);
        } else if (localName == "class") {
            hdl.characters = function(ch, start, length) {
                //console.log(">>> class : " + ch);
                if ( (ch == "object.container.storageFolder") ||
                     (ch == "object.container") ||
                     (ch == "object.container.album") )
                    e.type = "container";
                else if (ch == "object.container.genre.movieGenre")
                    e.type = "container_video";
                else if (ch == "object.container.album.musicAlbum" ||
                         ch == "object.container.genre.musicGenre")
                    e.type = "container_audio";
                else if (ch == "object.container.album.photoAlbum")
                    e.type = "container_photo";
                else if (ch == "object.item.videoItem")
                    e.type = "video_item";
                else if (ch == "object.item.audioItem.musicTrack")
                    e.type = "audio_item";
                else if ( (ch == "object.item.imageItem.photo") ||
                          (ch == "object.item.imageItem") )
                    e.type = "image_item";
            }
        } else if (localName == "res") {
            self.currentMetadata = new Metadata();

            hdl.characters = function(ch, start, length) { 
                self.currentMetadata.properties["url"] = ch; 
            }

            /* size, protocolInfo, duration, bitrate, bitsPerSample, sampleFrequency, nrAudioChannels
             * resolution, colorDepth, codec...
            */
            for (var i = 0 ; i < atts.getLength() ; i++) 
                self.currentMetadata.setProp(atts.getLocalName(i), atts.getValue(i));

            e.addMetadata(self.currentMetadata);
            
        } else if (localName == "desc") {
            // Windows Media Player Metadata extensions
            for (var i = 0 ; i < atts.getLength() ; i++)
                if (atts.getLocalName(i) == "id") {
                    var val = atts.getValue(i);
                    if (val == "artist") {
                        e.characters = function(ch, start, length) {
                            e.artistAlbumArtist = ch;
                        } 
                    }
                    /*else if (val == "author")
                    else if (val == "Year")
                    else if (val == "UserRating")
                    else if (val == "folderPath")*/
                }
        }
    }

    this.contentHandler.endElement = function(ch, start, length) {
        self.contentHandler.characters = DefaultHandler2.prototype.characters;
    }

	this.saxParser = XMLReaderFactory.createXMLReader();

	this.saxParser.setHandler(this.contentHandler);
	this.saxParser.parseString(xDidl);
}

DidlNode.prototype.getNodes = function() {
    return this.nodes;
}

// Metadata definition
var Metadata = function() {    
    this.properties = {};
}

Metadata.prototype.setProp = function(prop, value) {
    this.properties[prop] = value;
}

// Entry definition
var Entry = function(contentDirectoryIndex, id, parentNode, didl) {
    this.index = contentDirectoryIndex;
    this.id = id;
    this.parentNode = parentNode;
    this.didl = didl;
    this.properties = {};
    this.metadata = new Array();
}

Entry.prototype.setType = function(type) {
    this.type = type;
}

Entry.prototype.setProp = function(handler, prop) {
    var self = this;
    handler.characters = function(ch, start, length) {
        self.properties[prop] = ch;
    }
}

Entry.prototype.addMetadata = function(metadata) {
    this.metadata.push(metadata);
}

Entry.prototype.setTitle = function(title) {
    this.title = title;
    this.properties["title"] = title;
}

Entry.prototype.setIconURL = function(iconUrl) {
    this.iconUrl = iconUrl;
}

Entry.prototype.getIconURL = function() {
    if (this.type == "root") return this.iconUrl;
    else if (this.type == "container") return 'images/folder.png'; 
    else if (this.type == "container_video") return 'images/folder_video.png'; 
    else if (this.type == "container_audio") return 'images/folder_music.png'; 
    else if (this.type == "container_photo") return 'images/folder_photo.png'; 
    else if (this.type == "audio_item")      return this.properties["albumArtURI"];
    else if (this.type == "image_item")      return this.metadata[0] ? this.metadata[0].properties["url"] : "images/image.png";
    else if (this.type == "video_item") {
        for (var i=0; i<this.metadata.length; i++) {
            if (this.metadata[i].properties["protocolInfo"].indexOf("jpeg") != -1)
                return this.metadata[i].properties["url"];
        }
    }
}

Entry.prototype.getURL = function() {
    if (this.type == "audio_item") {
        for (var i=0; i<this.metadata.length; i++) {
            if (this.metadata[i].properties["protocolInfo"].indexOf("audio/mpeg") != -1) {
                return this.metadata[i].properties["url"];
            }
        }
    } else if (this.type == "image_item") {
        return this.metadata[0] ? this.metadata[0].properties["url"] : "images/image.png";
    } else if (this.type == "video_item") {
        for (var i=0; i<this.metadata.length; i++) {

            if ((this.metadata[i].properties["protocolInfo"].indexOf("video/mp4") != -1) ||
                (this.metadata[i].properties["protocolInfo"].indexOf("video/mpeg") != -1)) {
                this.format = "video/mp4";
                return this.metadata[i].properties["url"];
            } else if (this.metadata[i].properties["protocolInfo"].indexOf("video/ogg") != -1) {
                this.format = "video/ogg";
                return this.metadata[i].properties["url"];
            } else if (this.metadata[i].properties["protocolInfo"].indexOf("video/webm") != -1) {
                this.format = "video/webm";
                return this.metadata[i].properties["url"];
            } else if (this.metadata[i].properties["protocolInfo"].indexOf("video/x-ms-wmv") != -1) {
                this.format = "video/x-ms-wmv";
                return this.metadata[i].properties["url"];
            }
        }
        
        for (var i=0; i<this.metadata.length; i++) {
            if (this.metadata[i].properties["protocolInfo"].indexOf("video/") != -1) 
                return this.metadata[i].properties["url"];
        }
        return this.metadata[0].properties["url"];
    }

    return undefined;
}

Entry.prototype.getFormat = function() {
    return this.format;
}

Entry.prototype.getId = function() {
    return this.id;
}

Entry.prototype.getTitle = function() {
    return this.title;
}

Entry.prototype.toJSON = function() {
    return {    data  : this.properties["title"], 
                attr  : {   id      : this.id, 
                            rel     : this.type, 
                            index   : this.index,
                            didl    : this.didl,
                        },
                state : "closed"
                };
}
