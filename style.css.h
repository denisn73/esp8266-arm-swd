R"---(// style.css

body {
    color: black;
    background: white;
    white-space: nowrap;
}

a:link {
    text-decoration: underline;
    color: #003;
}
a:visited {
    color: #003;
}

.visible {
    opacity: 1;
    transition: opacity 0.1s linear;
}
.hidden {
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

)---"; // End of raw string
