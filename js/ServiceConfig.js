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

var _curServiceConfig;

var ServiceConfig = function(config, controlURL) {

    var self = this;
    _curServiceConfig = self;

    this.contentHandler = new DefaultHandler2();

    this.friendlyName = "";
    this.currentIconSize = 0;
    this.controlURL = controlURL;
    this.baseURL = undefined;
     
    this.contentHandler.startElement = function(namespaceURI, localName, qName, atts) {
        if (localName == "friendlyName") {
            self.contentHandler.characters = function(ch, start, length) {
                self.friendlyName = ch;
            }
        } else if (localName == "icon") {
            self.contentHandler.startElement = self.parseIcon; 
            self.contentHandler.endElement = self.endParseIcon; 
        } else if (localName == "controlURL") {
            self.contentHandler.characters = function(ch, start, length) {
                var off = self.controlURL.lastIndexOf(ch);
                if ((self.baseURL == undefined) && (off != -1)) {
                    self.baseURL = self.controlURL.substring(0, off);
                }
            }
        }
    }

    this.contentHandler.endElement = function(ch, start, length) {
        self.contentHandler.characters = DefaultHandler2.prototype.characters;
        self.contentHandler.startElement = self.defaultStartElement;
    }

    this.defaultStartElement = this.contentHandler.startElement;
    this.defaultEndElement = this.contentHandler.endElement;

	this.saxParser = XMLReaderFactory.createXMLReader();

	this.saxParser.setHandler(this.contentHandler);
	this.saxParser.parseString(config);
}

ServiceConfig.prototype.parseIcon = function(namespaceURI, localName, qName, atts) {
    var self = _curServiceConfig;
    if (localName == "mimetype") {
        self.contentHandler.characters = function(ch, start, length) {
            self.currentIconMimeType = ch;
        }
    } else if (localName == "width") {
        if (self.currentIconMimeType == "image/png") {
            self.contentHandler.characters = function(ch, start, length) {
                var size = parseInt(ch)
                if (size <= 80 && self.currentIconSize < size) {
                    //console.log("parseIcon select size " + ch);
                    self.currentIconSize = size;
                    self.iconUrl = undefined;
                }
            }
        }
    } else if (localName == "url") {
        if (self.currentIconMimeType == "image/png" &&
            self.currentIconSize != 0) {
            self.contentHandler.characters = function(ch, start, length) {
                self.iconUrl = ch;
            }
        }
    }
}

ServiceConfig.prototype.endParseIcon =  function(ch, start, length) {
    var self = _curServiceConfig;
    if (self.iconUrl) {
        self.contentHandler.characters = DefaultHandler2.prototype.characters;
        self.contentHandler.startElement = self.defaultStartElement;
        self.contentHandler.endElement = self.defaultEndElement;
    }
}
 
