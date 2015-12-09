#pragma once

static const char *kWebAppHeader = R"---(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<style type="text/css">

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

</style>
<script>

function targetAction(url, resultId)
{
    var req = new XMLHttpRequest();
    var element = document.getElementById(resultId);

    req.open('GET', url);
    req.responseType = 'json';

    element.className = 'hidden';
    element.textContent = 'pending';

    document.getElementById(resultId).textContent = '';
    req.addEventListener('load', function () {
        element.textContent = req.response ? 'OK!' : 'failed';
        element.className = 'visible';
        setTimeout( function() {
            element.className = 'hidden';
        }, 300);
    });

    req.send();
}

function targetReset()
{
    targetAction('/reset', 'targetResetResult');
}

function targetHalt()
{
    targetAction('/halt', 'targetHaltResult');
}

function toHex32(value)
{
    return ('00000000' + (value|0).toString(16)).slice(-8);
}

function hexDump(firstAddress, wordCount)
{
    // Emit a DOM scaffolding that we'll fill in asynchronously.
    // Assumes this is all still inside a <pre> environment.
    // Each span gets an ID of the form mem-%08x, referencing
    // its full address.

    var html = '';
    var numColumns = 8;
    var addr = firstAddress;

    for (var count = 0; count < wordCount;) {
        html += toHex32(addr) + ':';
        for (var x = 0; x < numColumns; x++, count++, addr += 4) {
            html += ' <span id="mem-' + toHex32(addr) + '">////////</span>'
        }
        html += '\n';
    }

    document.write(html);
}

</script>
</head>
<body>
<pre>)---";

static const char *kWebAppFooter = R"---(
</pre>
</body>
</html>
)---";
