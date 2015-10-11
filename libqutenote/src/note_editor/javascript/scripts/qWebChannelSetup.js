(function(){
    var baseUrl = "ws://127.0.0.1:" + window.websocketserverport.toString();
    console.log("Connecting to WebSocket server at " + baseUrl + ".");
    var socket = new WebSocket(baseUrl);

    socket.onclose = function() {
        console.error("web channel closed");
    };

    socket.onerror = function(error) {
        console.error("web channel error: " + error);
    };

    socket.onopen = function() {
        console.log("WebSocket connected, setting up QWebChannel.");
        new QWebChannel(socket, function(channel) {
            window.resourceCache = channel.objects.resourceCache;
            window.resourceCache.notifyResourceInfo.connect(onResourceInfoReceived);

            window.genericResourceImageHandler = channel.objects.genericResourceImageHandler;
            window.genericResourceImageHandler.genericResourceImageFound.connect(onGenericResourceImageReceived);

            window.iconThemeHandler = channel.objects.iconThemeHandler;
            window.iconThemeHandler.notifyIconFilePathForIconThemeName.connect(onIconFilePathForIconThemeNameReceived);

            window.pageMutationObserver = channel.objects.pageMutationObserver;
            window.enCryptElementClickHandler = channel.objects.enCryptElementClickHandler;
            window.openAndSaveResourceButtonsHandler = channel.objects.openAndSaveResourceButtonsHandler;
            window.textCursorPositionHandler = channel.objects.textCursorPositionHandler;
            window.contextMenuEventHandler = channel.objects.contextMenuEventHandler;

            window.mimeTypeIconHandler = channel.objects.mimeTypeIconHandler;
            window.mimeTypeIconHandler.notifyIconFilePathForMimeType.connect(function(mimeType, iconFilePath) {
                console.log("Response to the event of icon file path for mime type update: mime type = " +
                            mimeType + ", icon file path = " + iconFilePath);
                var genericResources = document.querySelectorAll('.resource-icon[type="' + mimeType + '"]');
                var numResources = genericResources.length;
                console.log("Found " + numResources + " generic resources for mime type " + mimeType +
                            "; icon file path = " + iconFilePath);
                var index = 0;
                for(; index < numResources; ++index) {
                    console.log("window.mimeTypeIconHandler.gotIconFilePathForMimeType.connect: index = ", index);
                    genericResources[index].setAttribute("src", iconFilePath.toString());
                }
            });
            console.log("Created window variables for objects exposed to JavaScript");
        });
        console.log("Connected to WebChannel, ready to send/receive messages!");
    }
})();
