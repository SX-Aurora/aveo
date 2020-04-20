SET(libexecdir	"${CMAKE_INSTALL_PREFIX}/libexec")
SET(libdir		"${CMAKE_INSTALL_PREFIX}/lib")
CONFIGURE_FILE(${CMAKE_CURRENT_LIST_DIR}/relink_aveorun.in ${CMAKE_CURRENT_BINARY_DIR}/relink_aveorun @ONLY)

ADD_CUSTOM_TARGET(relink_aveorun_symlink ALL
	COMMAND ${CMAKE_COMMAND} -E create_symlink "relink_aveorun" ${CMAKE_CURRENT_BINARY_DIR}/mk_veorun_static)

INSTALL(FILES 
	${CMAKE_CURRENT_LIST_DIR}/gen_aveorun_static_symtable
	${CMAKE_CURRENT_BINARY_DIR}/relink_aveorun
	${CMAKE_CURRENT_BINARY_DIR}/mk_veorun_static
	DESTINATION libexec PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE)