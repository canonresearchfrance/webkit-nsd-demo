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

var MediaTree = function() {
    this.entries = {};
}

MediaTree.prototype.isEmpty = function() {
    return jQuery.isEmptyObject(this.entries);
}

MediaTree.prototype.addList = function(id, nodes) {
    //console.log("MediaTree : addList : " + id + " node size " + nodes.length);
    this.entries[id] = nodes;
}

MediaTree.prototype.removeList = function(id) {
    //console.log("MediaTree : removeList : " + id);
    this.cleanup(id);
    delete this.entries[id];
}

MediaTree.prototype.getList = function(id) {
    //if (this.entries[id] != undefined)
    //    console.log("MediaTree : getList : " + id + " : size " + this.entries[id].length);
    return this.entries[id];
}

// Remove all children of an id (node container)
MediaTree.prototype.cleanup = function(id) {
    //console.log("MediaTree : cleanup : " + id);
    for (var item in this.entries) {
        if (this.entries.hasOwnProperty(item)) {

            var nodes = this.entries[item];
            // Check if the current node parent id is the id to clean.
            if (nodes[0].parentNode.id == id) {
                for (var i=0; i<nodes.length; i++) {
                    var type = nodes[i].type;
                    if (type == "container" ||
                        type == "container_video" ||
                        type == "container_audio" ||
                        type == "container_photo") {
                        //console.log("MediaTree : cleanup (push)");
                        // Recursively cleanup the container nodes.
                        this.cleanup(nodes[i].id);
                        //console.log("MediaTree : cleanup (pop)");
                        //console.log("    delete : " + nodes[i].id);
                        delete this.entries[nodes[i].id];
                    }
                }
            }
        }
    }
    //delete this.entries[id];
}

