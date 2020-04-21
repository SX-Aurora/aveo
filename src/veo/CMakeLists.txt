SET(CMAKE_C_COMPILER		/opt/nec/ve/bin/nfort)
SET(CMAKE_CXX_COMPILER		/opt/nec/ve/bin/nfort)
SET_DIRECTORY_PROPERTIES	(PROPERTIES COMPILE_OPTIONS "")
SET_DIRECTORY_PROPERTIES	(PROPERTIES LINK_OPTIONS "")

# AVEORUN ----------------------------------------------------------------------
SET(FILE_AVEORUN ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/aveorun)
SET(FILE_AVEORUN_FTRACE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/aveorun-ftrace)

ADD_EXECUTABLE			(aveorun $<TARGET_OBJECTS:aveorun_objs>)
#TARGET_COMPILE_OPTIONS	(aveorun PRIVATE -pthread -fopenmp -v -cxxlib)
TARGET_LINK_OPTIONS		(aveorun PRIVATE -pthread -fopenmp -v -cxxlib)
TARGET_LINK_LIBRARIES	(aveorun aveoVE_static veio dl ${VEURPC_OMP_STATIC})

ADD_EXECUTABLE			(aveorun_ftrace $<TARGET_OBJECTS:aveorun_ftrace_objs>)
#TARGET_COMPILE_OPTIONS	(aveorun_ftrace PRIVATE -pthread -ftrace -fopenmp -cxxlib)
TARGET_LINK_OPTIONS		(aveorun_ftrace PRIVATE -pthread -ftrace -fopenmp -cxxlib)
TARGET_LINK_LIBRARIES	(aveorun_ftrace aveoVE_static veio dl veftrace ${VEURPC_OMP_STATIC})

# Install ----------------------------------------------------------------------
INSTALL(TARGETS aveorun aveorun_ftrace RUNTIME DESTINATION libexec)