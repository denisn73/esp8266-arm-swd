R""//(
(function(){
"use strict";

// <swd-begin> initiates connection with the target if necessary.
//   When indeterminate, it is hidden.
//   If disconnected, the entire contents are replaced with an error
//   If successful, the contents are shown.
//   Also, optional info is written to the 'result-element'

var TargetInfo = document.registerElement('swd-begin', {
    prototype: Object.create(HTMLElement.prototype, {

        render: { value: function(json) {
            if (json.connected) {
                var resultsAttr = this.attributes['result-element'];
                if (resultsAttr) {
                    document.getElementById(resultsAttr.value).innerHTML = `
                        Connected to the ARM debug port. <br/>
                        Processor IDCODE: <b>${toHex32(json.idcode)}</b> <br/>
                        Detected extensions: ${json.detected} <br/>
                    `;
                }
            } else {
                this.innerHTML = `
                    Unfortunately, <br/>
                    I failed to connect to the debug port. <br/>
                    Check your wiring maybe? <br/>
                `;
            }
            this.style.display = null;
        }},

        createdCallback: { value: function() {
            var el = this;
            var req = new XMLHttpRequest();
            el.style.display = 'none';
            req.open('GET', '/api/begin');
            req.addEventListener('load', function () {
                el.render(JSON.parse(req.responseText));
            });
            req.send();
        }}
    })
});

// <swd-async-action> is a modified hyperlink that shows pass/fail asynchronously

var AsyncAction = document.registerElement('swd-async-action', {
    extends: 'a',
    prototype: Object.create(HTMLAnchorElement.prototype, {

        action: { value: function(json) {
            var el = this;
            var req = new XMLHttpRequest();

            el.resultElement.className = 'result-hidden';
            el.resultElement.textContent = '';

            req.open('GET', this.href);
            req.addEventListener('load', function () {
                var response = JSON.parse(req.responseText);
                el.resultElement.textContent = response ? ' OK! ' : ' failed ';
                el.resultElement.className = 'result-visible';
                setTimeout( function() {
                    el.resultElement.className = 'result-hidden';
                }, 300);
            });
            req.send();
        }},

        createdCallback: { value: function() {
            var el = this;

            this.addEventListener('click', function (evt) {
                el.action();
                evt.preventDefault();
            });

            this.resultElement = document.createElement('span');
            this.parentNode.insertBefore(this.resultElement, this.nextSibling);
            this.resultElement.className = 'result-hidden';
        }}
    })
});

// <swd-hexedit> is an editable hex dump with live updates

var Hexedit = document.registerElement('swd-hexedit', {
    prototype: Object.create(HTMLElement.prototype, {

        createdCallback: { value: function() {
            var el = this;
            el.innerText = 'hexes go here';
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


function toHex32(value)
{
    return ('00000000' + value.toString(16)).slice(-8);
}


})();
)"//";
