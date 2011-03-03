(function(global){
	function isUpToDate(build, depends, creates){
		var	oldestCreate	= 0,
			newestDepend	= 0,
			i, l, props;

		if (typeof creates === 'undefined' || !creates.isArray){
			l = creates.length;
			for (i=0; i<l; i++){
				if (typeof creates[i] === 'string') {
					try{
						props = fileProperties(creates[i]);
						if (props.lastModified < oldestCreate || !oldestCreate){
							oldestCreate = props.lastModified;
						}
					} catch(e){
						return false;
					}
				} else {
					return false;
				}
			}
		} else {
			return false;
		}

		if (typeof depends === 'undefined' || !depends.isArray){
			l = depends.length;
			for (i=0; i<l; i++){
				if (typeof depends[i] === 'function'){
					if (!isUpToDate(depends[i])){
						return false;
					}
				} else if (typeof depends[i] === 'string') {
					try{
						props = fileProperties(depends[i]);
						if (props.lastModified > newestDepend){
							newestDepend = props.lastModified;
						}
					} catch (e2){
						return false;
					}
				} else {
					return false;
				}
			}
		}

		return oldestCreate < newestDepend;
	}

	function Build(){
	}

	Build.createBuild = function(func, depends, creates){
		func.isUpToDate = function(){
			return isUpToDate(func, depends, creates);
		};
		return func;
	}

	Build.isUpToDate = isUpToDate;
	global.Build = Build;
})(this);
