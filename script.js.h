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

        createdCallback: { value: function() {
            var el = this;
            var req = new XMLHttpRequest();
            el.style.display = 'none';
            req.open('GET', '/api/begin');
            req.addEventListener('load', function () {
                el.render(JSON.parse(req.responseText));
            });
            req.send();
        }},

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
        }}
    })
});

// <swd-async-action> is a modified hyperlink that shows pass/fail asynchronously

var AsyncAction = document.registerElement('swd-async-action', {
    extends: 'a',
    prototype: Object.create(HTMLAnchorElement.prototype, {

        createdCallback: { value: function() {
            var el = this;

            this.addEventListener('click', function (evt) {
                el.action();
                evt.preventDefault();
            });

            this.resultElement = document.createElement('span');
            this.parentNode.insertBefore(this.resultElement, this.nextSibling);
            this.resultElement.className = 'result-hidden';
        }},

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
        }}
    })
});

// <swd-hexedit> is an editable hex dump with live updates

var Hexedit = document.registerElement('swd-hexedit', {
    prototype: Object.create(HTMLElement.prototype, {

        createdCallback: { value: function() {
            this.navigation = this.getAttribute('navigation') == 'true';
            this.headings = this.getAttribute('headings') != 'false';
            this.addr = parseInt(this.getAttribute('addr'), 0) || 0;
            this.count = parseInt(this.getAttribute('count'), 0) || 1024;
            this.columns = parseInt(this.getAttribute('columns'), 0) || 8;
            this.render();

            if (this.navigation) {
                // Render automaticaly when the nav parameters change
                var el = this;
                window.addEventListener('hashchange', function() {
                    el.render();
                });
            }
        }},

        render: { value: function() {
            // Keep nav parameters fresh on each render
            if (this.navigation) {
                var params = getHashParams();
                if ('addr' in params) this.addr = parseInt(params['addr'], 0);
                if ('count' in params) this.count = parseInt(params['count'], 0);
                if ('columns' in params) this.columns = parseInt(params['columns'], 0);
            }

            // Hex dump body, made of <swd-hexword> elements
            var parts = [];
            for (var i = 0, addr = this.addr; i < this.count;) {
                parts.push('<div>')
                if (this.headings) {
                    parts.push(toHex32(addr) + ':');
                }
                for (var col = 0; col < this.columns; col++, addr += 4, i++) {
                    parts.push(` <span is="swd-hexword" addr="${addr}"></span>`);
                }
                parts.push('</div>');
            }
            this.innerHTML = parts.join('');
        }}
    })
});

// <swd-hexnav> is a navigation link for use with <swd-hexedit navigation="true">.
// The 'nav-step' parameter indicates how many bytes to step forward or backward in memory

var Hexnav = document.registerElement('swd-hexnav', {
    extends: 'a',
    prototype: Object.create(HTMLAnchorElement.prototype, {

        createdCallback: { value: function() {
            var el = this;
            this.addEventListener('click', function (evt) {
                el.action();
                evt.preventDefault();
            });
        }},

        action: { value: function(json) {
            var el = this;
            var step = parseInt(this.getAttribute('nav-step'), 0) || 1;
            var addr = parseInt(getHashParams()['addr']) || 0;
            var newAddr = Math.max(addr + step, 0);
            setHashParam('addr', `0x${toHex32(newAddr)}`);
        }}
    })
});

// <swd-hexword> is a single live-updating editable hex word

var Hexword = document.registerElement('swd-hexword', {
    extends: 'span',
    prototype: Object.create(HTMLSpanElement.prototype, {

        createdCallback: { value: function() {
            this.markStale();
            this.addr = parseInt(this.getAttribute('addr'), 0);
            this.contentEditable = true;
            this.savedValue = null;
            this.renderTimestamp = null;
            this.updateOnBlur = false;

            this.addEventListener('keydown', function(event) {
                var el = event.target;
                if (event.keyCode == 13) {
                    // This is enter or shift+enter.
                    // Send the value immediately, even if it hasn't changed,
                    // but don't insert a newline.
                    event.preventDefault();
                    el.savedValue = null;
                    el.updateValue();
                    el.selectAll();
                }
            });

            this.addEventListener('input', function(event) {
                var el = event.target;
                if (el == document.activeElement) {
                    el.updateOnBlur = true;
                } else {
                    el.updateValue();
                }
            });

            this.addEventListener('focus', function(evt) {
                // Select the whole word, for convenient keyboard editing
                evt.target.selectAll();
            });

            this.addEventListener('blur', function(evt) {
                // Implicit send if the value changed when navigating away
                var el = event.target;
                if (el.updateOnBlur) {
                    el.updateOnBlur = false;
                    el.updateValue();
                }
            });
        }},

        selectAll: { value: function(json) {
            var el = this;
            requestAnimationFrame(function() {
                var range = document.createRange();
                range.selectNodeContents(el);
                var sel = window.getSelection();
                sel.removeAllRanges();
                sel.addRange(range);
            });
        }},

        markStale: { value: function() {
            // The valid is unknown for now
            this.textContent = '........';
            this.className = 'mem-stale';
            this.renderTimestamp = null;
        }},

        render: { value: function(word) {
            // We have some memory or 'null' as an error marker
            var oldTimestamp = this.renderTimestamp;
            this.renderTimestamp = Date.now();
            if (word == null) {
                this.textContent = '////////';
                this.className = 'mem-error';
            } else {
                var textValue = toHex32(word);
                var oldValue = this.textContent;
                element.textContent = textValue;
                if (oldTimestamp == null) {
                    // First read after it was invalid
                    element.className = 'mem-okay';
                } else if (oldValue == textValue) {
                    element.className = 'mem-stable';
                } else {
                    element.className = 'mem-changing';
                }
            }
        }},

        updateValue: { value: function() {
            // Clean up the entered hex value, and transmit if it doesn't match savedValue

            var oldValue = this.savedValue;
            var newValue = parseInt(this.textContent, 16);
            if (isNaN(newValue)) {
                newValue = oldValue || 0;
            }

            this.textContent = toHex32(newValue);
            this.savedValue = newValue;

            if (newValue != oldValue) {
                this.className = 'mem-pending';
                this.textContent = toHex32(newValue);

                // Asynchronous write. Usually results will be apparent due to the
                // periodic read cycle, but we also try to keep the style updated
                // to indicate confirmed completion of the memory store operation.

                var url = `/api/store?0x${toHex32(this.addr)}=0x${toHex32(newValue)}`;
                var el = this;
                var req = new XMLHttpRequest();
                req.open('GET', url);
                req.addEventListener('load', function () {
                    var result = JSON.parse(req.responseText)[0].result;
                    if (el.className == 'mem-pending') {
                        el.className = result ? 'mem-okay' : 'mem-error';
                    }
                });
                req.send();
            }
        }}
    })
});


function getHashParams() {
    // Parse key-value params from the fragment portion of the URI.
    // http://stackoverflow.com/a/4198132

    var hashParams = {};
    var e,
        a = /\+/g,  // Regex for replacing addition symbol with a space
        r = /([^&;=]+)=?([^&;]*)/g,
        d = function (s) { return decodeURIComponent(s.replace(a, ' ')); },
        q = window.location.hash.substring(1);

    while (e = r.exec(q)) {
       hashParams[d(e[1])] = d(e[2]);
    }
    return hashParams;
}

function setHashParam(key, value) {
    // Change one value from the fragment portion of the URI, leaving the others.

    var parts = window.location.hash.substring(1).split('&');
    for (var i = 0; i < parts.length; i++) {
        if (parts[i].startsWith(key + '=')) {
            parts[i] = `${key}=${encodeURIComponent(value)}`;
        }
    }
    window.location.hash = '#' + parts.join('&');
}

function toHex32(value) {
    return ('00000000' + value.toString(16)).slice(-8);
}

})();
)"//";
