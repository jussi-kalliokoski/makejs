import('conditional.js');

//console.log(makejs.flags);

function all()
{
	echo('pro');
	console.log(makejs.argumentedFlags['-h']);
	console.log(makejs.argumentedFlags['-h'].length);
}

function onfinish()
{
	echo('done');
}
