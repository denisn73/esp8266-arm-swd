R""//(

body {
    font-family: monospace;
    color: black;
    background: white;
    white-space: nowrap;
    margin: 1em;
}

p {
    margin: 1em 0;
}

a:link {
    text-decoration: underline;
    color: #003;
}
a:visited {
    color: #003;
}

.result-visible {
    opacity: 1;
    transition: opacity 0.1s linear;
}
.result-hidden {
    opacity: 0;
    transition: opacity 2s ease;
}

.mem-stale {
    color: #888;
}
.mem-loading {
    background: #ddd;
}
.mem-changing {
    font-weight: bold;
}
.mem-error {
    background: #fcc;
}
.mem-pending {
    background: #ff6;
}

)"//";
