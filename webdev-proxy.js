#!/usr/bin/env node

//
// Development proxy, for working on the web frontend
//
// The dev/test cycle for the Arduino code is pretty slow, so this module
// aims to speed up the frontend development by serving the static files
// directly and proxying other requests to the real server.
//

var TARGET_HOST = 'esp8266-swd.local',
    SKETCH_PATH = '.',
    HTTP_PORT = 8266;

var http = require('http'),
    fs = require('fs');

function localFilePath (url) {
    var localPath = SKETCH_PATH + url + '.h';
    return /^\/\w+\.\w+$/.test(url) && fs.existsSync(localPath) && localPath;
}

function unwrapHeader(contents) {
    return String(contents).trim().split('\n').slice(1, -1).join('\n');
}

var server = http.createServer(function(req, res) {
    var logInfo = req.method + ' ' + req.url;
    var localPath = localFilePath(req.url);

    if (localPath) {
        fs.readFile(localPath, function (err, data) {
            if (err) throw err;
            res.writeHead(200, {'Content-Type': 'text/javascript'});
            res.end(unwrapHeader(data));
            console.log(logInfo + ' <--file-- ' + localPath);
        });

    } else {
        console.log(logInfo + ' proxy...');
        http.request({
            hostname: TARGET_HOST,
            port: 80,
            path: req.url,
            method: req.method,
            agent: false
        }, function (proxy, err) {
            console.log(logInfo + ' = ' + proxy.statusCode);

            if (err) {
                res.writeHead(500, {'Content-Type': 'text/plain'});
                res.end('Proxy error');
            } else {
                res.writeHead(proxy.statusCode, proxy.headers);
                proxy.on('data', function(chunk) {
                    res.write(chunk);
                }));
                proxy.on('end', function() {
                    res.end();
                }));
            }
        }).end();
    }
});
 
server.listen(HTTP_PORT);
console.log('http://localhost:' + HTTP_PORT);
