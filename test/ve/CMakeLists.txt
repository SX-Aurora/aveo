SET(CMAKE_C_COMPILER		/opt/nec/ve/bin/ncc)
SET(CMAKE_CXX_COMPILER		/opt/nec/ve/bin/nc++)
SET_DIRECTORY_PROPERTIES	(PROPERTIES COMPILE_OPTIONS "")
ADD_COMPILE_OPTIONS			(-fdiag-inline=0 -fdiag-parallel=0 -fdiag-vector=0 -O4 -no-proginf -no-perfcnt)

SET(VE_TESTS
	vehello
	vehello2
	vestackargs
	veexcept
	veasyncmem
	vetestomp
	vearith_ftrace
)

FOREACH(TEST ${VE_TESTS})
	ADD_LIBRARY(${TEST} SHARED ${CMAKE_CURRENT_LIST_DIR}/../lib${TEST}.c)
	TARGET_COMPILE_OPTIONS(${TEST} PRIVATE -fopenmp)
	TARGET_LINK_OPTIONS(${TEST} PRIVATE -fopenmp)
	INSTALL(TARGETS ${TEST} LIBRARY DESTINATION tests)
ENDFOREACH()