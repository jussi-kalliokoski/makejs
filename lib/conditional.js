Conditional =
(function(){

	return {
		parseJS: function(code, flags)
		{
			return parseConditional.call(this, ['/*# ', '*/'], code, flags);
		},
		parseHTML: function(code, flags)
		{
			return parseConditional.call(this, ['<?cjs', '?>'], code, flags);
		}
	};


	function parseConditional (tags, code, flags)
	{
		var pos = 0, target = 0, result = '', i, func;
		result = 'func=function(){var f={},result=\'\';'
		if (flags)
			for (i=0; i<flags.length; i++)
				result += 'f.'+flags[i]+'=true;';
		while (pos < code.length)
		{
			target = code.indexOf(tags[0], pos)
			if (target == -1)
			{
				result += ' result+=\'' + parse(code.substr(pos)) + '\';';
				break;
			}
			result += ' result+=\'' + parse(code.substr(pos, target-pos)) + '\';';
			pos = target + 4;
			target = code.indexOf(tags[1], pos);
			if (target < -1)
				throw("***Conditional: Unclosed CONDITIONAL tag at "+pos+'');
			result += code.substr(pos, target - pos);
			pos = target+2;
		}
		result += ' return result}';
		try{
			eval(result);
		}catch(e){
			throw('***Conditional: Syntax error (' + e + ');');
		}
		return func;

		function parse(str)
		{
			return str.split('\\').join('\\\\').split('\'').join('\\\'').split('\n').join('\\n');
		}
	}
})();
