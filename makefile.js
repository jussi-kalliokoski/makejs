function all()
{
	echo('pro');
	save('pro.js', 'cockoo');
	compile();
}

function compile()
{
	shell("g++ -o makejs makejs.cc -lv8");
}

function install()
{
	shell("cp bin/makejs /usr/bin/");
}

function uninstall()
{
	shell("rm /usr/bin/makejs");
}

function clean()
{
	shell("rm bin/makejs");
}
