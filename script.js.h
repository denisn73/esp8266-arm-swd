R"---(// script.js

function getHashParams()
{
    // Parse key-value params from the fragment portion of the URI.
    // http://stackoverflow.com/a/4198132

    var hashParams = {};
    var e,
        a = /\+/g,  // Regex for replacing addition symbol with a space
        r = /([^&;=]+)=?([^&;]*)/g,
        d = function (s) { return decodeURIComponent(s.replace(a, " ")); },
        q = window.location.hash.substring(1);

    while (e = r.exec(q))
       hashParams[d(e[1])] = d(e[2]);

    return hashParams;
}

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

function targetReset() { targetAction('/api/reset', 'targetResetResult'); }
function targetHalt() { targetAction('/api/halt', 'targetHaltResult'); }

function toHex32(value) {
    return ('00000000' + value.toString(16)).slice(-8);
}

var kStaleMemory = '........';
var kBadMemory   = '////////';

// List of memory addresses we're asynchronously refreshing
var asyncMemoryElements = [];

// List of addresses still waiting on data in the current refresh cycle.
var elementUpdatesPending = [];

// When we scroll, deleting the pending updates helps us get to the new data faster
window.addEventListener('scroll', function() {
    elementUpdatesPending = [];
});

function memElementId(addr) {
    return 'mem-' + toHex32(addr);
}

function hexDump(firstAddress, wordCount)
{
    // Returns an HTML scaffolding that we'll fill in asynchronously.
    // Assumes this is all still inside a <pre> environment.

    var html = '';
    var numColumns = 8;
    var addr = firstAddress;

    for (var count = 0; count < wordCount;) {
        html += toHex32(addr) + ':';
        for (var x = 0; x < numColumns; x++, count++, addr += 4) {
            var id = memElementId(addr);
            html += ` <span 
                        contenteditable="true"
                        data-addr="${addr}"
                        class="mem-stale" id="${id}"
                        onkeydown="hexDump_keydown(event)"
                        oninput="hexDump_input(event)"
                        onfocus="hexDump_focus(event)"
                        onblur="hexDump_blur(event)"
                        >${kStaleMemory}</span>`;
            asyncMemoryElements.push(addr);
        }
        html += '\n';
    }
    return html;
}

function hexDumpPager_nav(id, event, addr, count)
{
    // Keep the URI fragment updated with current settings in canonical format
    window.location.hash = `addr=0x${toHex32(addr)}&count=${count}`;
    document.getElementById(id).innerHTML = hexDumpPager_render(id);
    event.preventDefault();

    // Kickstart the update loop
    refreshTargetMemory();
}

function hexDumpPager_render(containerId)
{
    var params = getHashParams();
    var addr = parseInt(params['addr'], 0) || 0;
    var count = parseInt(params['count'], 0) || 1024;

    function linkto(addr, count, text)
    {
        return `<a href="#" onclick="hexDumpPager_nav('${containerId}', event, ${addr}, ${count})">${text}</a>`;
    }

    var navbar = (
        '\n' +
        linkto(addr - count/4, count, '&lt;--') + ' ' +
        linkto(addr - 64*1024/4, count, '&lt;&lt;64k') + ' ' +
        linkto(addr - 8*1024/4, count, '&lt;&lt;8k') + ' ' +
        linkto(addr - 1*1024/4, count, '&lt;&lt;1k') + ' ' +
        '--- ' +
        linkto(addr + 1*1024/4, count, '1k>>') + ' ' +
        linkto(addr + 8*1024/4, count, '8k>>') + ' ' +
        linkto(addr + 64*1024/4, count, '64k>>') + ' ' +
        linkto(addr + count/4, count, '-->') + '\n\n'
    );

    // Assumes the pager is the only thing we have going on...
    asyncMemoryElements = [];

    return navbar + hexDump(addr, count) + navbar;
}

function hexDumpPager(containerId)
{
    containerId = containerId || 'hexDumpPager';
    return `<div id="${containerId}">${hexDumpPager_render(containerId)}</span>`
}

function hexDump_keydown(event)
{
    if (event.keyCode == 13) {
        // This is enter or shift+enter.
        // Send the value immediately, even if it hasn't changed,
        // but don't insert a newline.

        event.preventDefault();
        updateHexElement(event.target, true);
    }
}

function hexDump_input(event)
{
    // The editable content has changed. Usually we want to wait until blur.

    if (event.target != document.activeElement) {
        updateHexElement(event.target);
    }
}

function hexDump_focus(event)
{
    // Select the whole word by default when entering with the 'tab' key.
    // See http://stackoverflow.com/questions/3805852/select-all-text-in-contenteditable-div-when-it-focus-click

    requestAnimationFrame(function() {
        var range = document.createRange();
        range.selectNodeContents(event.target);
        var sel = window.getSelection();
        sel.removeAllRanges();
        sel.addRange(range);
    });
}

function hexDump_blur(event)
{
    updateHexElement(event.target);
}

function updateHexElement(element, updateEvenIfUnchanged)
{
    var addr = parseInt(element.dataset.addr, 10);

    // Immediately clean up the value.
    // Note that we revert to saved value on parse error

    var oldValue = parseInt(element.dataset.value, 10);
    if (isNaN(oldValue)) oldValue = 0;
    var value = parseInt(element.textContent, 16);
    if (isNaN(value)) value = oldValue;

    element.textContent = toHex32(value);
    element.dataset.value = value;

    if (updateEvenIfUnchanged || value != oldValue) {
        element.className = 'mem-pending';

        // Fire-and-forget write. The read cycle will act as a confirmation.
        var req = new XMLHttpRequest();
        req.open('GET', `/api/store?${addr}=${value}`);
        req.send();
    }
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

    var isElementInView;
    if (document.hidden) {
        // The whole thing is invisible, for example if the window is minimized.
        // This shouldn't cause any new requests to go out.
        isElementInView = function() { return false; }

    } else {
        // Look up the body's clientrect once per refresh,
        // and look up each element's rect only once ever.
        // getBoundingClientRect() is too slow to call each time.

        var bodyTop = document.body.getBoundingClientRect().top;
        var winHeight = document.documentElement.clientHeight;

        isElementInView = function(element) {
            if (element.savedTop == undefined) {
                var rect = element.getBoundingClientRect();
                element.savedTop = rect.top - bodyTop;
                element.savedBottom = rect.bottom - bodyTop;
            }
            return (element.savedBottom + bodyTop >= 0 &&
                    element.savedTop + bodyTop <= winHeight);
        }
    }

    // Each word of target memory in our web page is a span element...
    asyncMemoryElements.forEach( function (addr, index) {
        var element = document.getElementById(memElementId(addr));

        if (Date.now() - element.dataset.timestamp < minimumRefreshPeriodMillisec) {
            // Our cached timestamp indicates that we've updated this item
            // so recently that we shouldn't do anything with it.
            return;
        }

        if (isElementInView(element)) {

            // Consider this element for the next async request.
            if (collectElements) {
                elementUpdatesPending.push(addr);
            }

        } else {
            // Old and invisible; invalidate the cached contents

            element.className = 'mem-stale';
            element.dataset.timestamp = undefined;
        }
    });

    // Now if we're making a new request, sort the pending list and pick the first contiguous group
    if (!targetMemRequest && elementUpdatesPending.length) {

        // Keep the array sorted by ascending address
        elementUpdatesPending.sort(function(a, b) { return a - b; });

        // Remove anything that has scrolled out of view, or is being edited
        var activeElement = document.activeElement;
        elementUpdatesPending = elementUpdatesPending.filter( function( addr ) {
            var el = document.getElementById(memElementId(addr));
            return isElementInView(el) && el != activeElement;
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
        req.open('GET', `/api/load?addr=${firstAddr}&count=${wordCount}`);
        req.addEventListener('load', function () {

            var newTimestamp = Date.now();
            var response = JSON.parse(req.responseText);

            // Display the results
            response.forEach( function (value, index) {
                var addr = firstAddr + index * 4;
                var element = document.getElementById(memElementId(addr));
                if (element) {
                    element.dataset.timestamp = newTimestamp;
                    element.dataset.value = value;

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

)---"; // End of raw string