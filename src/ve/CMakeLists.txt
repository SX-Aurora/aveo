SET(CMAKE_C_COMPILER		/opt/nec/ve/bin/ncc)
SET(CMAKE_CXX_COMPILER		/opt/nec/ve/bin/nc++)
SET_DIRECTORY_PROPERTIES	(PROPERTIES COMPILE_OPTIONS "")
ADD_COMPILE_OPTIONS			(-fdiag-inline=0 -fdiag-parallel=0 -fdiag-vector=0 -O4 -no-proginf -no-perfcnt)

# AVEO Libraries ---------------------------------------------------------------
ADD_LIBRARY				(aveoVE SHARED ${VE_SRC})
TARGET_LINK_LIBRARIES	(aveoVE ${VEURPC_OMP_SHARED} dl veio veftrace pthread)

ADD_LIBRARY				(aveoVE_static STATIC ${VE_SRC})
SET_TARGET_PROPERTIES	(aveoVE_static PROPERTIES OUTPUT_NAME aveoVE)

ADD_LIBRARY				(aveorun_objs OBJECT ${VEO_SRC})
TARGET_COMPILE_OPTIONS	(aveorun_objs PRIVATE -fopenmp)
ADD_LIBRARY				(aveorun_ftrace_objs OBJECT ${VEO_SRC})
TARGET_COMPILE_OPTIONS	(aveorun_ftrace_objs PRIVATE -fopenmp -ftrace)

# Install ----------------------------------------------------------------------
INSTALL(TARGETS aveoVE aveoVE_static LIBRARY DESTINATION lib)