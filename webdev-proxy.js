#!/usr/bin/env node

//
// Development proxy, for working on the web frontend
//
// The dev/test cycle for the Arduino code is pretty slow, so this module
// aims to speed up the frontend development by serving the static files
// directly and proxying other requests to the real server.
//

var TARGET_HOST = 'esp8266-swd.local',
    DATA_PATH = './data',
    HTTP_PORT = 8266;

var TYPES = {
    js: 'text/javascript',
    html: 'text/html',
    css: 'text/css'
};

var http = require('http'),
    path = require('path'),
    dns = require('dns'),
    open = require('open'),
    fs = require('fs');

console.log('Resolving target host, ' + TARGET_HOST);
dns.lookup(TARGET_HOST, function (err, address, family) {
    if (err) {
        console.log(err);
        return;
    }
    TARGET_HOST = address; // Cache the DNS lookup
    console.log('Hi there, ' + TARGET_HOST);
    http.createServer(handleRequest).listen(HTTP_PORT);
    var url = 'http://localhost:' + HTTP_PORT;
    console.log(url);
    open(url);
});

function localFilePath (url) {
    var localPath = DATA_PATH + url;
    return /^\/\w+\.\w+$/.test(url) && fs.existsSync(localPath) && localPath;
}

function handleRequest(req, res) {
    var logInfo = req.method + ' ' + req.url;
    var localPath = localFilePath(req.url);

    if (localPath) {
        // Handled locally
        fs.readFile(localPath, function (err, data) {
            if (err) throw err;
            var type = TYPES[path.extname(req.url).substr(1)] || 'text/plain';
            res.writeHead(200, {'Content-Type': type});
            res.end(data);
            console.log(logInfo + ' <--file-- ' + localPath + ' (' + type + ')');
        });

    } else {
        // Handled via proxy
        http.get({
            hostname: TARGET_HOST,
            path: req.url,
            agent: false
        },
        function (proxy) {
            console.log(logInfo + ' proxy ' + proxy.statusCode);
            res.writeHead(proxy.statusCode, proxy.headers);
            proxy.on('data', function(chunk) { res.write(chunk); });
            proxy.on('end', function() { res.end(); });
        }).on('error', function (e) {
            res.writeHead(500, {'Content-Type': 'text/plain'});
            res.end('Proxy error: ' + e.message);
        });
    }
}
