R""//(<!doctype html>
<html lang="en">
<head>
    <meta charset="utf-8">
    <link rel="stylesheet" type="text/css" media="all" href="/style.css" />
    <script src="/polyfill.js" type="text/javascript"></script>
    <script src="/script.js" type="text/javascript"></script>

	<title>swd</title>
</head>
<body>

<p>
	Howdy, neighbor!<br/>
	Nice to <a href="https://github.com/scanlime/esp8266-arm-swd">meet you</a>.
</p>

<swd-target-info></swd-target-info>
<p>
	&gt; <a is='swd-async-action' href="/api/reset">reset</a><br/>
	&gt; <a is='swd-async-action' href="/api/halt">debug halt</a>
</p>

<p>
	Some RAM, live and editable: (<a href="/mem#addr=0x1fffff00">more RAM</a>)
</p>
<swd-hexedit addr="0x1fffff00" count="256"></swd-hexedit>

<p>
	Memory mapped GPIOs: (<a href='/mmio'>more memory mapped hardware</a>)
</p>
<swd-hexedit addr="0x400ff000" count="16"></swd-hexedit>

<p>
	Some flash memory: (<a href='/mem')>more flash</a>)
</p>
<swd-hexedit addr="0" count="256"></swd-hexedit>

</body>
</html>
)"//";
