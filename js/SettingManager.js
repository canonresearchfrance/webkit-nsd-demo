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

var SettingManager = function() {
    this.renderingControls = {};

    this.variables = {};
}
 
SettingManager.prototype.init = function() {
}

SettingManager.prototype.deinit = function() {
    for (var i in this.renderingControls)
        this.renderingControls[i].initialize = false;
}

SettingManager.prototype.addRenderingControl = function(service) {
    var id = service.id.split(':')[1];

    // check if the servive already exist
    if (this.renderingControls[id] != undefined) {
        this.renderingControls[id].initialize = true;
        return;
    }

    var self = this;
    service.onavailable = function() { self.onavailable(this); };
    service.onunavailable = function() { self.onavailable(this); };

    //console.log("RenderingControl added " + service.url);
    this.renderingControls[id] = this.createRenderingControl(service);
}

SettingManager.prototype.onavailable = function(service) {
    //console.log("RenderingControl service " + (service.online ? " on": "off") +  "line : " + service.url);
    CURRENT_SERVICES_AVAILABLE += service.online ? 1 : -1;
}

SettingManager.prototype.createRenderingControl = function(service) {
    var renderingControl;
    
    var version = service.type.split(':')[5];
    
    if (version == 1)
        renderingControl = new CpProxySchemasUpnpOrgRenderingControl1(service.id, service.url);
    else if (version == 2)
        renderingControl = new CpProxySchemasUpnpOrgRenderingControl2(service.id, service.url);

    renderingControl.initialize = true;

    return renderingControl;
}

SettingManager.prototype.getListPresets = function() {
    var self = this;
    renderingControl.ListPresets(0, 
        function(result) {
            self.listPresets = result.split(';');
        },
        function(errorMsg, msg) { console.log("ListPresets failed : " + errorMsg);
        });
}

SettingManager.prototype.getRenderingControl = function(id) {
    if (this.renderingControls[id] == undefined)
        console.log("RenderingControl not found");
        
    return this.renderingControls[id];
}

SettingManager.prototype.getVolume = function(id) {
    var renderingControl = this.getRenderingControl(id);

    if (renderingControl)
        renderingControl.GetVolume(0, "Master", 
            function(result) {
                $('#slider-volume').val(result["CurrentVolume"]);
                $('#slider-volume').slider('refresh');
            },
            function(errorMsg, msg) {console.log("GetVolume failed: " + errorMsg);});
}

SettingManager.prototype.setVolume = function(id, volume) {
    var renderingControl = this.getRenderingControl(id);
    
    if (renderingControl)
        renderingControl.SetVolume(0, "Master", volume,
            function(result) {/*console.log("SetVolume : " + result);*/}, 
            function(errorMsg, msg) {console.log("SetVolume failed: " + errorMsg);});
}

SettingManager.prototype.getMute = function(id) {
    var renderingControl = this.getRenderingControl(id);

    if (renderingControl)
        renderingControl.GetMute(0, "Master", 
            function(result) {
                $('#muteButton').val(result["CurrentMute"] == true ? 1 : 0);
                $('#muteButton').slider('refresh');
            },
            function(errorMsg, msg) {console.log("GetMute failed: " + errorMsg);});
}

SettingManager.prototype.setMute = function(id, mute) {
    var renderingControl = this.getRenderingControl(id);
    
    if (renderingControl)
        renderingControl.SetMute(0, "Master", mute,
            function(result) {/*console.log("SetMute : " + result);*/}, 
            function(errorMsg, msg) {console.log("SetMute failed: " + errorMsg);});
}

SettingManager.prototype.setVariable = function(handler, variable) {
    var self = this;
    handler.characters = function(ch, start, length) {
        self.variables[variable] = ch;
    }

}

var StateVariable = function(settingManager, xStateVariables) {

    var self = this;

    this.contentHandler = new DefaultHandler2();
    this.settingManager = settingManager;

    this.contentHandler.startElement = function(namespaceURI, localName, qName, atts) {
        if (localName == "stateVariable")  {
            self.settingManager.setVariable(self.contentHandler);
        }
    }

    this.saxParser = XMLReaderFactory.createXMLReader();

	this.saxParser.setHandler(this.contentHandler);
	this.saxParser.parseString(xStateVariables);    
}
