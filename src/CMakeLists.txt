SET(VH_SRC
	${CMAKE_CURRENT_LIST_DIR}/ProcHandle.cpp
	${CMAKE_CURRENT_LIST_DIR}/Context.cpp
	${CMAKE_CURRENT_LIST_DIR}/AsyncTransfer.cpp
	${CMAKE_CURRENT_LIST_DIR}/Command.cpp
	${CMAKE_CURRENT_LIST_DIR}/CallArgs.cpp
	${CMAKE_CURRENT_LIST_DIR}/veo_api.cpp
	${CMAKE_CURRENT_LIST_DIR}/veo_urpc.c
	${CMAKE_CURRENT_LIST_DIR}/veo_urpc_vh.cpp
)

SET(VE_SRC
	${CMAKE_CURRENT_LIST_DIR}/veo_urpc.c
	${CMAKE_CURRENT_LIST_DIR}/veo_urpc_ve.c
)

SET(VEO_SRC
	${CMAKE_CURRENT_LIST_DIR}/aveorun.c
)

ADD_DEFINITIONS(-DAVEO_VERSION_STRING="${AVEO_VERSION}")

# Device -----------------------------------------------------------------------
ADD_SUBDIRECTORY(ve)
ADD_SUBDIRECTORY(veo)

# Host -------------------------------------------------------------------------
SET(VEORUN_BIN ${CMAKE_INSTALL_PREFIX}/libexec/aveorun)
ADD_DEFINITIONS(-DVEORUN_BIN="${VEORUN_BIN}")

ADD_LIBRARY(aveoVH SHARED ${VH_SRC})
TARGET_LINK_LIBRARIES(aveoVH ${VEURPC_HOST_SHARED} dl)
TARGET_LINK_OPTIONS(aveoVH PRIVATE -Wl,--version-script=${CMAKE_CURRENT_LIST_DIR}/libaveoVH.map)

ADD_LIBRARY(aveoVH_static STATIC ${VH_SRC})
SET_TARGET_PROPERTIES(aveoVH_static PROPERTIES OUTPUT_NAME aveoVH)

ADD_CUSTOM_TARGET(aveoVH_symlink ALL
	COMMAND ${CMAKE_COMMAND} -E create_symlink "libaveoVH.so" ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libveo.so)

INSTALL(TARGETS aveoVH aveoVH_static LIBRARY DESTINATION lib)
INSTALL(FILES ${CMAKE_CURRENT_LIST_DIR}/ve_offload.h DESTINATION include)
INSTALL(FILES ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libveo.so DESTINATION lib)
