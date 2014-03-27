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

var Player = function(settingManager) {
    this.avTransports = {};

    this.settingManager = settingManager;

    this.renderers = new Array();

    var self = this;

    // Init controls
    $('#slider-volume').bind( "change", function(event, ui) {
        self.setVolume($(this).val());
    });

    $('#muteButton').bind( "change", function(event, ui) {
        self.setMute(parseInt($(this).val()));
    });

}

Player.prototype.init = function() {
    if (!this.renderers.length)
        return;

    this.buildCombo();
}

Player.prototype.deinit = function() {
    for (var i in this.avTransports)
        this.avTransports[i].initialize = false;
}

Player.prototype.updateAVTransport = function(avTransport, url, config) {
    avTransport.url = url;
    avTransport.initialize = true;
}

Player.prototype.addAVTransport = function(service) {
    var id = service.id.split(':')[1];
    var config = new ServiceConfig(service.config, service.url);

 	// check if the servive already exist
    if (this.avTransports[id] != undefined) {
        this.updateAVTransport(this.avTransports[id], service.url, config);
        return;
    }

    //console.log("AVTransport added " + service.url);
    this.avTransports[id] = this.createAVTransport(service);
    this.addToCombo(config.friendlyName, id);

    var self = this;
    service.onavailable = function() { self.onavailable(this); };
    service.onunavailable = function() { self.onavailable(this); };
}

Player.prototype.onavailable = function(service) {
    var id = service.id.split(':')[1];
    var config = new ServiceConfig(service.config, service.url);

    //console.log("AVTransport service " + (service.online ? " on": "off") +  "line : " + service.url);
    if (this.avTransports[id] != undefined) {
        this.updateAVTransport(this.avTransports[id], service.url, config);
        for (var i=0; i<this.renderers.length; i++) 
            if (this.renderers[i].id == id)
                this.renderers[i].disable = service.online ? false : true;
    }
    CURRENT_SERVICES_AVAILABLE += service.online ? 1 : -1;

    this.buildCombo();
}

Player.prototype.addToCombo = function(label, id) {
    this.renderers.push({label: label, disable: false, id: id});
}

Player.prototype.buildCombo = function() {
    var html = '<div role="heading" class="ui-controlgroup-label">\n';
    html+= '<h2>Select a remote Media Renderer</h2></h2></div>\n';

    $('#listFieldset').empty();
    $('#listFieldset').append(html);
    for (var i=0; i<this.renderers.length; i++) {
        var disable = this.renderers[i].disable;
        html = '<input type="radio" name="list-renderer" id="list-choice-' + i + '" ';
        html+= 'value="' + this.renderers[i].id + '" ';
        if(disable)
            html+= 'style="pointer-events: none; display : none" ';
        if (i == 0)
            html+= 'checked="checked" ';
        html+= '/>\n';
        html+= '<label data-theme="b" id="list-label-' + i + '" for="list-choice-' + i + '" style="height: 75px;';
        if(disable)
            html+= 'pointer-events: none; opacity : 0.3';
        html+= '">' + this.renderers[i].label + '</label>\n';
        $('#listFieldset').append(html);

        if (disable)
            continue;

        var self = this;
        var id = "list-choice-" + i;
        $("input[id="+id+"]").bind( "change", function(event, ui) {
            self.getVolume();
            self.getMute();
        });
    }
    $('#listFieldset').trigger('create');
}

Player.prototype.createAVTransport = function (service) {
    var avTransport;
    var version = service.type.split(':')[5];

    if (version == 1)
        avTransport = new CpProxySchemasUpnpOrgAVTransport1(service.id, service.url);
    else if (version == 2)
        avTransport = new CpProxySchemasUpnpOrgAVTransport2(service.id, service.url);

    avTransport.initialize = true;

    return avTransport;
}

Player.prototype.getCurrentAVTransportID = function() {
    // Get id from list UI
    return $('input[name=list-renderer]:checked').val();
}

Player.prototype.getAVTransport = function() {
    return this.avTransports[this.getCurrentAVTransportID()];
}

Player.prototype.setNode = function(node) {
    this.node = node;
    if (node.type == "image_item") {
        $('#player-container').html('<img src="' + node.getIconURL() + '"/>');
        $('#volume-controller').hide();
    } else if (node.type == "audio_item") {
        $('#player-container').html('<img src="' + node.getIconURL() + '"/>');
        $('#volume-controller').show();
        this.getVolume();
        this.getMute();
    } else if (node.type == "video_item") {
        var html = '<video width="853" height="480" controller="false" id="local-video" '
        html+= 'poster="' + node.getIconURL() + '">';
        // Do not provide format : html+= '<source src='+ node.getURL() + ' type=' + node.getFormat() + '/>';
        html+= '<source src='+ node.getURL()+ '/>';
        html+= '</video>';
        $('#player-container').html(html);
        $('#local-video').simpleVideo();
        $('#volume-controller').show();
        this.getVolume();
        this.getMute();
    }
}

Player.prototype.play = function() {
    var avTransport = this.getAVTransport();
    var self = this;
    
    avTransport.SetAVTransportURI(0, this.node.getURL(), this.node.didl,
        function(msg) {
            //console.log("SetAVTransportURI succeed");
            avTransport.Play(0, "1",
                function(msg) { 
                    //console.log("Play succeed"); 
                    if (self.node.type != "image_item")
                        self.infoInterval = setInterval(function() {
                            self.getPositionInfo();
                        }, 1000);
                }, 
                function(errorMsg, msg) { console.log("Play failed : " + errorMsg);
            });
        }, 
        function(errorMsg, msg) {
            console.log("SetAVTransportURI failed errorMsg : " + errorMsg);
        });
}

Player.prototype.pause = function() {
    var avTransport = this.getAVTransport();
    var self = this;
                                                                             
    avTransport.Pause(0, 
        function(msg) { 
            //console.log("Pause succeed"); 
            clearInterval(self.infoInterval);
            }, 
        function(errorMsg, msg) { console.log("Pause failed : " + errorMsg);
        });
}

Player.prototype.stop = function() {
    var avTransport = this.getAVTransport();
    var self = this;
                                                                            
    avTransport.Stop(0,
        function(msg) { 
            //console.log("Stop succeed"); 
            clearInterval(self.infoInterval);
            $('#slider-position').val(0);
            $('#slider-position').slider('refresh');
        }, 
        function(errorMsg, msg) { console.log("Stop failed : " + errorMsg);
    });
}

Player.prototype.getPositionInfo = function() {
    var avTransport = this.getAVTransport();
    var self = this;

    avTransport.GetPositionInfo(0,
        function(result) {
            var duration = result["TrackDuration"].split(':');
            var time = result["RelTime"].split(':');

            duration = parseInt(duration[0])*3600 + parseInt(duration[1])*60 + parseInt(duration[2]);
            time = parseInt(time[0])*3600 + parseInt(time[1])*60 + parseInt(time[2]);
            time = time * 100 / duration;
        
            // The remote player stop the media
            if (self.currentTime && self.currentTime > 0 && time == 0)
                clearInterval(self.infoInterval);

            self.currentTime = time;

            $('#slider-position').val(time);
            $('#slider-position').slider('refresh');

            var html = '<h2>Position : ';
            html += result["RelTime"];
            html += ' / ';
            html += result["TrackDuration"];
            html += '</h2>';
            $('#player-timing').html(html);

        },
        function(errorMsg, msg) { console.log("GetPositionInfo failed : " + errorMsg);
    });
}

Player.prototype.getVolume = function() {
    this.settingManager.getVolume(this.getCurrentAVTransportID()); 
}

Player.prototype.setVolume = function(volume) {
    this.settingManager.setVolume(this.getCurrentAVTransportID(), volume); 
}

Player.prototype.getMute = function() {
    this.settingManager.getMute(this.getCurrentAVTransportID()); 
}

Player.prototype.setMute = function(mute) {

    this.settingManager.setMute(this.getCurrentAVTransportID(), mute);
}

