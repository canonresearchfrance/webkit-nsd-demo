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

var Explorer = function() {
    this.contentDirectories = new Array();
    this.index = 0;

    this.root_nodes = new Array();
    this.currentNodes = this.root_nodes;
    this.currentParentNode = undefined;
    this.all_mediatree = {};
}

Explorer.prototype.init = function(info, metadataCallback, mediaSelectedCallback) {
    this.info = info;
    this.metadataCallback = metadataCallback;
    this.mediaSelectedCallback = mediaSelectedCallback;
    
    // Cleanup contentDirectories
    for (var i = 0; i < this.contentDirectories.length; i++) {
        var contentDir = this.contentDirectories[i];
        if (contentDir.initialize == false) {
            // cleanup media list browse
            var mediatree = this.all_mediatree[contentDir.url];
            if (mediatree)
                mediatree.cleanup(0);
            delete this.all_mediatree[contentDir.url];
           
            // Disable contentDir from root nodes
            contentDir.node.disable = true;
        }
    }

    this.buildRootList(this);
}

Explorer.prototype.deinit = function() {
    this.info = undefined;
    this.metadataCallback = undefined;
    this.mediaSelectedCallback = undefined;
    for (var i = 0; i < this.contentDirectories.length; i++)
        this.contentDirectories[i].initialize = false;
    for (var i = 0; i < this.root_nodes.length; i++) 
        this.root_nodes[i].disable = false;

}

Explorer.prototype.updateContentDirectory= function(contentDir, url, config) {
    contentDir.url = url;
    contentDir.node.setTitle(config.friendlyName);
    contentDir.node.setIconURL(config.baseURL + config.iconUrl);
    contentDir.initialize = true;
}

Explorer.prototype.addContentDirectory= function(service) {
    var config = new ServiceConfig(service.config, service.url);
    var contentDir;

    // check if the servive already exist
    for (var i = 0; i < this.contentDirectories.length; i++) {
        contentDir = this.contentDirectories[i];
        if (service.id == contentDir.id) {
            // update contentDir 
            this.updateContentDirectory(contentDir, service.url, config);
            return;
        }
    }

    contentDir = this.createContentDirectory(service);

    //console.log("Service " + config.friendlyName +" : ContentDirectory version " + service.type.split(':')[5]);
    var self = this;
    service.onavailable = function() { self.onavailable(this); };
    service.onunavailable = function() { self.onavailable(this); };
    // Subscribe UPnP events
    service.onnotify = function(msg) { self.onnotify(this, msg); };
 
    var entry = new Entry(this.index, 0, undefined, null);
    entry.setType("root");
    entry.contentDir = contentDir;
    entry.setTitle(config.friendlyName);
    entry.setIconURL(config.baseURL + config.iconUrl);
    this.root_nodes.push(entry);
    
    //console.log("ContentDirectory added " + service.url);
    contentDir.node = entry;
    this.contentDirectories[this.index] = contentDir;
    this.index++;
}

Explorer.prototype.onavailable = function(service) {
    var config = new ServiceConfig(service.config, service.url);

    //console.log("ContentDirectory service " + (service.online ? " on": "off") +  "line : " + service.url);
    
    for (var i = 0; i < this.contentDirectories.length; i++) {
        contentDir = this.contentDirectories[i];
        if (service.id == contentDir.id) {
            // update contentDir 
            this.updateContentDirectory(contentDir, service.url, config);
            contentDir.node.disable = service.online ? false : true;
            break;
        }
    }
    CURRENT_SERVICES_AVAILABLE += service.online ? 1 : -1;

    if (this.root_nodes == this.currentNodes)
        this.buildRootList(this);
}

Explorer.prototype.onnotify = function(service, msg) {
    var index;
    var needRefresh = 0;
    var evt = new ServiceEvent(msg.data);
    var containerId = evt.ContainerUpdateIDs.split(',')[0];
    var mediatree = this.all_mediatree[service.url];
    var config = new ServiceConfig(service.config, service.url);
    var medialist;

    //console.log("onnotify mediatree : " + mediatree);

    if (mediatree == undefined) {        
        $('#page-footer').show();
        $('#page-footer .ui-btn-text').text("Media server " + config.friendlyName + " content update");
        return;
    }

    medialist = mediatree.getList(containerId);

    // Mediatomb notify all containers updated. 
    if (medialist == undefined) {
        $('#page-footer').show();
        $('#page-footer .ui-btn-text').text("Media server " + config.friendlyName + " content update");
        return;
    }
    
    // If the container is currently display we need to update 
    // the list to provide a graphical feedback
    if (medialist == this.currentNodes)
        needRefresh = 1;
    
    for(index=0; index<this.contentDirectories.length; index++)
        if (this.contentDirectories[index].url == service.url)
            break;

    var pathUpdate = "";
    var node = medialist[0].parentNode;
    while (node.parentNode != undefined) {
        if (pathUpdate == "")
            pathUpdate = node.properties["title"];
        else 
            pathUpdate = node.properties["title"] + '/' + pathUpdate;
        node = node.parentNode;
    }        
    $('#page-footer .ui-btn-text').text("Media server " + node.properties["title"] + " folder update /" + pathUpdate);
    $('#page-footer').show();
                                                                                     
    mediatree.removeList(containerId);
    if (needRefresh) 
        this.browseEntry(index, containerId, medialist[0].parentNode, this.buildList, this);
    else
        this.buildRootList(this);
}

Explorer.prototype.createContentDirectory = function(service) {
    var contentDirectory;
    
    var version = service.type.split(':')[5];
    
    if (version == 1)
        contentDirectory = new CpProxySchemasUpnpOrgContentDirectory1(service.id, service.url);
    else if (version == 2)
        contentDirectory = new CpProxySchemasUpnpOrgContentDirectory2(service.id, service.url);
    else if (version == 3)
        contentDirectory = new CpProxySchemasUpnpOrgContentDirectory3(service.id, service.url);

    contentDirectory.initialize = true;

    return contentDirectory;
}

Explorer.prototype.browseEntry = function(index, id, parentNode, callback, _this) {
    var self = this;
    var contentDir = this.contentDirectories[index];
                                                                                        
    this.currentParentNode = parentNode;
                                                                                        
    // Retrieve the node list from the cache
    if (id == 0) {
        if (this.all_mediatree[contentDir.url] == undefined)
            this.all_mediatree[contentDir.url] = new MediaTree();
        else if (!this.all_mediatree[contentDir.url].isEmpty()) {
            this.buildList(this, this.all_mediatree[contentDir.url].getList(0));
            return;
        }
    } else {
        var medialist = this.all_mediatree[contentDir.url].getList(id);
        if (medialist) {
            this.buildList(this, medialist);
            return;
        }            
    }
                                                                                        
    //console.log("BrowseDirectChildren: " + id);
    contentDir.Browse(id, "BrowseDirectChildren", "@childcount", "0", "0", "+dc:title",
        function(msg) {
            var idx;
            //console.log("URL : " + contentDir.url);
            //console.log("[NumberReturned] : " + msg["NumberReturned"]);
            //console.log("[TotalMatches]   : " + msg["TotalMatches"]);
            //console.log("[UpdateID]       : " + msg["UpdateID"]);
            
            if (msg["NumberReturned"] == 0) {
                self.displayStatusBarMessage("Empty folder", 2);
                return;
            }
                                               
            for (var idx = 0; idx < self.contentDirectories.length; idx++) 
                if (contentDir == self.contentDirectories[idx])
                    break;                            
            var nodes = self.parseDIDL(idx, msg["Result"]);

            self.addList(idx, nodes);
    
            if (callback)
                callback(self, nodes);
        },
        function(errorMsg, msg) {
            console.log("ERROR : Browse children failed :" + errorMsg + "\nstatus : " + msg.status);
            delete self.all_mediatree[contentDir.url];
            self.all_mediatree[contentDir.url] = undefined; 
        });
}

Explorer.prototype.browseMetadata = function(index, id) {
    var self = this;
    var contentDir = this.contentDirectories[index];
                                                                          
    //console.log("BrowseMetadata: " + id);
    contentDir.Browse(id, "BrowseMetadata", "*", "0", "0", "+dc:title",
        function(msg) {
            //console.log("URL : " + contentDir.url);
            //console.log("[NumberReturned] : " + msg["NumberReturned"]);
            //console.log("[TotalMatches]   : " + msg["TotalMatches"]);
            //console.log("[UpdateID]       : " + msg["UpdateID"]);
            for (var idx = 0; idx < self.contentDirectories.length; idx++) 
                if (contentDir == self.contentDirectories[idx])
                    break;                            
            var node = self.parseDIDL(idx, msg["Result"]);
            
            if (self.metadataCallback)
                self.metadataCallback(self.info, node[0]);

            if (self.mediaSelectedCallback)
                self.mediaSelectedCallback(node[0]);
        },
        function(errorMsg, msg) {
            console.log("Browse metadata failed : " + errorMsg);
        });
}

Explorer.prototype.parseDIDL = function(index, xDIDL) {
    var didlNode = new DidlNode(this.currentParentNode, index, xDIDL);
    var nodes = didlNode.getNodes();
    this.currentNodes = nodes;
    return nodes;
}

Explorer.prototype.addList = function(index, nodes) {
    var mediatree = this.all_mediatree[this.contentDirectories[index].url];
    mediatree.addList(this.currentParentNode.id, nodes);
}

Explorer.prototype.buildRootList = function(_this) {
    this.buildList(_this, _this.root_nodes);
    $("#browseButton").children('span')[0].childNodes[0].innerText = "Browse";
}

Explorer.prototype.buildList = function(_this, nodes) {

    this.currentNodes = nodes;
    if (nodes == this.root_nodes) {
        this.currentParentNode = undefined;
        $('#folderTitle').text("UPnP Servers");
    } else {
        this.currentParentNode = nodes[0].parentNode;
        $('#folderTitle').text(this.currentParentNode.properties["title"]);
    }

    $('#medialist').children('li').unbind('touchstart mousedown');
    $('#medialist').empty();
    for (var i = 0; i < nodes.length; i++) {
        var n = nodes[i];
        var html = '<li id="';
        html+= n.id;
        if (n.type == "root") {
            html+= '" title="';
            html+= n.properties["title"];
        }
        if (n.disable == true)
            html+= '" style="pointer-events: none; opacity : 0.3"';
        html+= '" parentId="';
        if (n.parentNode == undefined) html+= '-1';
        else html+= n.parentNode.id;
        html+= '" index="';
        html+= n.index;
        html+= '" type="';
        html+= n.type;
        html+= '"><a href='+ "#" + ' data-transition="pop"> <img src="';
        html+= n.getIconURL(); 
        html+= '">'; 
        html+= n.properties["title"];
        html+= '</a></li>'; 

        $("#medialist").append(html);
    }
    $("#medialist").listview('refresh');
    $("#browseButton").children('span')[0].childNodes[0].innerText = "Back";

    // Bind click 
    var self = _this;
    $("#medialist").children('li').bind('touchstart mousedown', function(e) {
        var type = $(this).attr('type');
        var id = $(this).attr('id');
        if (type == "root") {
            var parentNode = self.getParentNodeFromTitle($(this).attr('title'));
            self.browseEntry($(this).attr('index'), id, parentNode, self.buildList, self);
        } else if (type.indexOf("item") == -1) {
            var parentNode = self.getParentNodeFromId(id);
            self.browseEntry($(this).attr('index'), id, parentNode, self.buildList, self);
        } else {
            self.browseMetadata($(this).attr('index'), id);
        }
    });
}

Explorer.prototype.getParentNodeFromTitle = function(title) {
    for (var i=0; i<this.currentNodes.length; i++) {
        if (title == this.currentNodes[i].properties["title"])
            return this.currentNodes[i];
    }
}

Explorer.prototype.getParentNodeFromId = function(id) {
    for (var i=0; i<this.currentNodes.length; i++) {
        if (id == this.currentNodes[i].id)
            return this.currentNodes[i];
    }
}

Explorer.prototype.backMediaList = function() {
    var item = $("#medialist").children('li');
    var parentId = item[0] ? item[0].getAttribute('parentId') : -1;
    var parentIdInt = parseInt(parentId);
    var index = item[0] ? item[0].getAttribute('index') : -1;
    
    if (parentId == -1) {
        return;
    } else if (parentIdInt != 0) {
        var medialist = this.all_mediatree[this.contentDirectories[index].url].getList(parentId);
        if (medialist) {
            medialist = this.all_mediatree[this.contentDirectories[index].url].getList(medialist[0].parentNode.parentNode.id);
            this.buildList(this, medialist);
        }            
    } else if (parentIdInt == 0)
        this.buildRootList(this);
}

Explorer.prototype.displayStatusBarMessage = function(msg, duration) {
    var self = this;
    $('#page-footer').show();
    $('#page-footer .ui-btn-text').text(msg);
    if (this.statusInterval == undefined) {
        this.statusInterval = setInterval(function() {
            clearInterval(self.statusInterval);
            self.statusInterval = undefined;
            $('#page-footer').hide();
        }, duration * 1000);
    }
}
