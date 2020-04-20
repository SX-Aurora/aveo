SET(VH_TESTS
	test_callsync
	test_callasync
	test_stackargs
	test_nprocs
	test_veexcept
	bandwidth
	latency
	call_latency
	test_getsym
	test_child1
	test_child2
	test_async_mem
	test_thread_main_call_race
	test_2ctx_callasync
	test_omp
	test_omp_static
	test_arith_ftrace
)

SET(VE_TESTS
	vehello
	vehello2
	vestackargs
	veexcept
	veasyncmem
	vetestomp
	vearith_ftrace
)

ADD_SUBDIRECTORY(ve)

FOREACH(TEST ${VH_TESTS})
	ADD_EXECUTABLE(${TEST} ${CMAKE_CURRENT_LIST_DIR}/${TEST}.c)
	TARGET_LINK_LIBRARIES(${TEST} aveoVH pthread)
	INSTALL(TARGETS ${TEST} RUNTIME DESTINATION tests)
ENDFOREACH()

INSTALL(FILES 
	${CMAKE_CURRENT_LIST_DIR}/run_tests.sh
	${CMAKE_CURRENT_LIST_DIR}/scan_bandwidth.sh
	${CMAKE_CURRENT_LIST_DIR}/scan_call_latency.sh
	DESTINATION tests
	PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE
)