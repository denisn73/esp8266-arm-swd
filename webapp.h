#pragma once

static const char *kWebAppHeader = R"---(<!doctype html>
<html lang="en">
<head>
    <meta charset="utf-8">
    <link rel="stylesheet" type="text/css" media="all" href="/style.css" />
    <script src="/script.js" type="text/javascript"></script>
</head>
<body onload='refreshTargetMemory()'>
<pre>)---";

static const char *kWebAppFooter = R"---(
</pre>
</body>
</html>
)---";

static const char *kWebAppStyle = R"---(
body
{
    color: black;
    background: white;
}

a:link
{
    text-decoration: underline;
    color: #002;
}

a:visited
{
    color: #002;
}

.visible
{
    opacity: 1;
    transition: opacity 0.1s linear;
}

.hidden
{
    opacity: 0;
    transition: opacity 2s ease;
}

.mem-stale
{
    color: #888;
}

.mem-loading
{
    background: #ddd;
}

.mem-changing
{
    font-weight: bold;
}

.mem-error
{
    background: #fcc;
}

)---";

static const char *kWebAppScript = R"---(

function targetAction(url, resultId)
{
    // This is a utility for fire-and-kinda-forget async actions.
    // They ping a URL which is expected to return a JSON bool,
    // which we use to stuff a result in the indicated element.

    var req = new XMLHttpRequest();
    var element = document.getElementById(resultId);

    req.open('GET', url);

    element.className = 'hidden';
    element.textContent = 'pending';

    document.getElementById(resultId).textContent = '';
    req.addEventListener('load', function () {
        var response = JSON.parse(req.responseText);
        element.textContent = response ? 'OK!' : 'failed';
        element.className = 'visible';
        setTimeout( function() {
            element.className = 'hidden';
        }, 300);
    });

    req.send();
}

function targetReset() { targetAction('/reset', 'targetResetResult'); }
function targetHalt() { targetAction('/halt', 'targetHaltResult'); }

function toHex32(value)
{
    return ('00000000' + value.toString(16)).slice(-8);
}

var kStaleMemory = '........';
var kBadMemory   = '////////';

// List of memory addresses we're asynchronously refreshing
var asyncMemoryElements = [];

// List of addresses still waiting on data in the current refresh cycle
var elementUpdatesPending = [];


function memElementId(addr)
{
    return 'mem-' + toHex32(addr);
}

function isElementInView(element)
{
    // We only care about vertical scroll, and we want to know if it's touching at all
    var rect = element.getBoundingClientRect();
    var winHeight = window.innerHeight || document.documentElement.clientHeight;
    return (rect.bottom >= 0 && rect.top <= winHeight);
}

function hexDump(firstAddress, wordCount)
{
    // Emit a DOM scaffolding that we'll fill in asynchronously.
    // Assumes this is all still inside a <pre> environment.

    var html = '';
    var numColumns = 8;
    var addr = firstAddress;

    for (var count = 0; count < wordCount;) {
        html += toHex32(addr) + ':';
        for (var x = 0; x < numColumns; x++, count++, addr += 4) {
            html += ' <span contenteditable="true" class="mem-stale" id="' + memElementId(addr) + '">'
                + kStaleMemory + '</span>';
            asyncMemoryElements.push(addr);
        }
        html += '\n';
    }

    document.write(html);
}

var targetMemRequest = null;
var targetMemTimer = null;

function refreshTargetMemory()
{
    // Using one asynchronous request at a time, keep the on-screen
    // async memory elements up to date.

    var minimumRefreshPeriodMillisec = 100;

    if (targetMemRequest && targetMemRequest.readyState == 4) {
        // Existing request is done; let it go.
        targetMemRequest = null;
    }

    // If our list of pending updates has run dry, refill it.
    var collectElements = elementUpdatesPending.length == 0;

    // Each word of target memory in our web page is a span element...
    asyncMemoryElements.forEach( function (addr, index) {
        var element = document.getElementById(memElementId(addr));

        if (Date.now() - element.targetMemTimestamp < minimumRefreshPeriodMillisec) {
            // Our cached timestamp indicates that we've updated this item
            // so recently that we shouldn't do anything with it.
            return;
        }

        if (!document.hidden && isElementInView(element)) {

            // Consider this element for the next async request.
            if (collectElements) {
                elementUpdatesPending.push(addr);
            }

        } else {
            // Old and invisible; invalidate the cached contents

            element.className = 'mem-stale';
            element.targetMemTimestamp = undefined;
        }
    });

    // Now if we're making a new request, sort the pending list and pick the first contiguous group
    if (!targetMemRequest && elementUpdatesPending.length) {

        // Keep the array sorted by ascending address
        elementUpdatesPending.sort(function(a, b) { return a - b; });

        // Remove anything that has scrolled out of view
        elementUpdatesPending = elementUpdatesPending.filter( function( addr ) {
            return isElementInView(document.getElementById(memElementId(addr)));
        });

        var firstAddr = elementUpdatesPending[0];
        var wordCount = 0;
        var maxWordCount = 32;   // Keep the requests smallish

        for (var index = 0; index < elementUpdatesPending.length && wordCount < maxWordCount; index++) {
            var addr = firstAddr + index * 4;
            if (elementUpdatesPending[index] == addr) {

                // This one is contiguous; include in this next request
                wordCount++;

                // Also, take this opportunity to mark it visually as 'loading'
                var element = document.getElementById(memElementId(addr));
                element.className = 'mem-loading';

            } else {
                break;
            }
        }
        elementUpdatesPending = elementUpdatesPending.slice(wordCount);

        var req = new XMLHttpRequest();
        targetMemRequest = req;
        req.open('GET', '/load?addr=' + firstAddr + '&count=' + wordCount);
        req.addEventListener('load', function () {

            var newTimestamp = Date.now();
            var response = JSON.parse(req.responseText);

            // Display the results
            response.forEach( function (value, index) {
                var addr = firstAddr + index * 4;
                var element = document.getElementById(memElementId(addr));
                if (element) {
                    element.targetMemTimestamp = newTimestamp;

                    if (value == null) {
                        // Couldn't read this memory
                        element.textContent = kBadMemory;
                        element.className = 'mem-error';

                    } else {
                        // Got a value! Diff it.

                        var textValue = toHex32(value);
                        var oldValue = element.textContent;
                        element.textContent = textValue;
                        
                        if (oldValue == kStaleMemory) {
                            element.className = 'mem-okay';
                        } else if (oldValue == textValue) {
                            element.className = 'mem-stable';
                        } else {
                            element.className = 'mem-changing';
                        }
                    }
                }
            });

            // Trigger another request maybe. (Decide what to highlight, if anything, before ending this event cycle)
            refreshTargetMemory();
        });

        // Break the event cycle here; seems to be necessary on webkit
        setTimeout(function() {
            req.send()
        }, 0);
    }

    // We try to chain requests when each finishes, but also poll as a retry in case a request is lost.
    if (targetMemTimer) {
        clearTimeout(targetMemTimer);
    }
    targetMemTimer = setTimeout(refreshTargetMemory, minimumRefreshPeriodMillisec / 2);
}

)---";
