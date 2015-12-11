R""//(
(function(){
"use strict";


var TargetInfo = document.registerElement('swd-target-info', {
    prototype: Object.create(HTMLElement.prototype, {
        createdCallback: { value: function() {
            this.innerText = "floo";
        }}
    })
});


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



})();
)"//";
