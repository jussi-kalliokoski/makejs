import('conditional.js');
import('lib/build.js');

function _verbosePrint(obj, silent){
	var out = '', i;
	switch (typeof obj){
		case 'string':
			out = '"' + obj.replace(/"/g, function(){ return '\\"'; }) + '"';
			break;
		case 'object':
			out = '{\n';
			for (i in obj){
				if (obj.hasOwnProperty(i)){
					out += '\t"' + i.replace(/"/g, function(){ return '\\"'; }) + '": ';
					out += _verbosePrint(obj[i], true);
					out += ',\n';
				}
			}
			if (out.length > 2){
				out = out.substr(0, out.length - 2) + '\n';
			}
			out += '}';
			break;
		default:
			out = obj.toString ? obj.toString() : obj;
	}
	if (!silent){
		console.log(out);
	}
	return out;
}

function all(){
	if (all.isUpToDate()){
		console.log('Up to date.');
	}
	var dir = shell('ls', true).split('\n');
	dir.splice(dir.length - 1, 1);
	_verbosePrint(fileProperties('makefile.js'));
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

Build.createBuild(all, ['bin'], ['makejs.cc']);
