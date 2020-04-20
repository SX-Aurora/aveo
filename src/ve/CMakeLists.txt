SET(CMAKE_C_COMPILER		/opt/nec/ve/bin/ncc)
SET(CMAKE_CXX_COMPILER		/opt/nec/ve/bin/nc++)
SET_DIRECTORY_PROPERTIES	(PROPERTIES COMPILE_OPTIONS "")
ADD_COMPILE_OPTIONS			(-fdiag-inline=0 -fdiag-parallel=0 -fdiag-vector=0 -O4 -no-proginf -no-perfcnt)

ADD_LIBRARY				(aveoVE SHARED ${VE_SRC})
TARGET_LINK_LIBRARIES	(aveoVE ${VEURPC_OMP_SHARED} dl veio veftrace pthread)

ADD_LIBRARY				(aveoVE_static STATIC ${VE_SRC})
SET_TARGET_PROPERTIES	(aveoVE_static PROPERTIES OUTPUT_NAME aveoVE)

ADD_EXECUTABLE			(aveorun ${VEO_SRC})
TARGET_COMPILE_OPTIONS	(aveorun PRIVATE -fopenmp)
TARGET_LINK_OPTIONS		(aveorun PRIVATE -fopenmp)
TARGET_LINK_LIBRARIES	(aveorun ${VEURPC_OMP_STATIC} veio veftrace pthread aveoVE_static)

ADD_EXECUTABLE			(aveorun-ftrace ${VEO_SRC})
TARGET_COMPILE_OPTIONS	(aveorun-ftrace PRIVATE -fopenmp -ftrace)
TARGET_LINK_OPTIONS		(aveorun-ftrace PRIVATE -fopenmp -ftrace)
TARGET_LINK_LIBRARIES	(aveorun-ftrace veio pthread aveoVE_static ${VEURPC_OMP_SHARED})

INSTALL(TARGETS aveoVE aveoVE_static LIBRARY DESTINATION lib)
INSTALL(TARGETS aveorun aveorun-ftrace RUNTIME DESTINATION libexec)