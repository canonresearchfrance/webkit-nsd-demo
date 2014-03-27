function getReadableFileSizeString(fileSizeInBytes) {

    var i = -1;
    var byteUnits = [' kB', ' MB', ' GB', ' TB', 'PB', 'EB', 'ZB', 'YB'];
    do {
        fileSizeInBytes = fileSizeInBytes / 1024;
        i++;
    } while (fileSizeInBytes > 1024);

    return Math.max(fileSizeInBytes, 0.1).toFixed(1) + byteUnits[i];
}

function displayMetadata(node) {
	// Success callback for network service discovery
	var metadataHTML = "<table style='border: 1px #000000 solid; border-collapse: collapse;'>\n";
	
	metadataHTML+= "<tr>\n";
	metadataHTML+= "<td style='border-bottom: 1px #000000 solid; padding: 5px; width: 700px;'>\n";
	metadataHTML+= "<p><b>Properties #" + node.properties["title"] + "</b>\n";
	metadataHTML+= "<ul>\n";
    for (var i in node.properties)
        metadataHTML+= "<li><b> "+ i + " :</b> " + node.properties[i] + "</li>\n";
	metadataHTML+= "</ul>\n";
	metadataHTML+= "</p>\n";
	metadataHTML+= "<p><b>Metadata #" + node.properties["title"] + "</b>\n";
	metadataHTML+= "<ul>\n";
    for (var i=0; i<node.metadata.length; i++) {
        for (var j in node.metadata[i].properties)
             metadataHTML+= "<li><b> "+ j + " :</b> " + node.metadata[i].properties[j] + "</li>\n";
	    metadataHTML+= "<br>\n";

    }
	metadataHTML+= "</ul>\n";
	metadataHTML+= "</p>\n";
	metadataHTML+= "</td>\n";
	metadataHTML+= "</tr>\n";

	metadataHTML+= "</table>\n";

	document.getElementById('metadata').innerHTML = metadataHTML;
}
