--
-- Copyright (c) 2017 Milos Tosic. All rights reserved.   
-- License: http://www.opensource.org/licenses/BSD-2-Clause
--

function projectDependencies_MTunerInject()
	return { "rdebug", "rbase" }
end

function projectAdd_MTunerInject() 
	addProject_cmd("MTunerInject")
end
