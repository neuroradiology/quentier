(function(){
    var genericResourceIcons = document.querySelectorAll(".resource-icon");
    console.log("Found " + genericResourceIcons.length.toString() + " generic resource icon tags");
    for(var index = 0; index < genericResourceIcons.length; ++index)
    {
        var currentResourceIcon = genericResourceIcons[index];
        var mimeType = currentResourceIcon.getAttribute("type");
        if (!mimeType) {
            console.log("Detected resource with undetermined mime type");
            continue;
        }

        mimeTypeIconHandler.iconFilePathForMimeType(mimeType.toString());
    }
})();