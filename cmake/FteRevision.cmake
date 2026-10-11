#Computes the SVNREVISION/SVNDATE/FTE_BRANCH defines the engine prints in its version string.
#Ported from the upstream script; the +29 bias keeps numbers consistent with the old svn history.
#Result: FTE_REVISION_DEFINES (list of preprocessor definitions), SVNREVISION, FTE_DATE, FTE_BRANCH.

set(FTE_REVISION_DEFINES "")
find_package(Git QUIET)

if(GIT_FOUND AND EXISTS "${FTE_ROOT}/.git")
	macro(_fte_git out)
		execute_process(COMMAND "${GIT_EXECUTABLE}" ${ARGN}
			WORKING_DIRECTORY "${FTE_ROOT}"
			OUTPUT_VARIABLE ${out}
			ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
	endmacro()

	_fte_git(_fte_describe describe --always --long --dirty)
	_fte_git(FTE_DATE log -1 --format=%cs)
	_fte_git(FTE_BRANCH branch --show-current)
	_fte_git(_fte_shallow rev-parse --is-shallow-repository)

	if(_fte_shallow STREQUAL "true")
		message(STATUS "Shallow clone prevents calculation of revision number.")
		set(SVNREVISION "git-${_fte_describe}")
	else()
		_fte_git(_fte_count rev-list HEAD --count)
		if(NOT _fte_count)
			set(_fte_count 0)
		endif()
		math(EXPR _fte_count "${_fte_count} + 29")
		if(FTE_BRANCH STREQUAL "master" OR FTE_BRANCH STREQUAL "")
			set(SVNREVISION "${_fte_count}-git-${_fte_describe}")
		else()
			set(SVNREVISION "${FTE_BRANCH}-${_fte_count}-git-${_fte_describe}")
		endif()
	endif()

	message(STATUS "FTE GIT ${FTE_BRANCH} Revision ${SVNREVISION}, ${FTE_DATE}")
	set(FTE_REVISION_DEFINES SVNREVISION=${SVNREVISION} SVNDATE=${FTE_DATE} FTE_BRANCH=${FTE_BRANCH})
endif()
