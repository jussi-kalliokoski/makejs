import('conditional.js');

function all(){
	var dir = shell('ls', true).split('\n');
	dir.splice(dir.length - 1, 1);
	console.log(dir);
	try{
		shell('cp strange/range hange pange', false);
		console.log('This is not how it\'s supposed to work... :/');
	} catch(e){
	}
}

function onfinish(){
	echo('done');
}
