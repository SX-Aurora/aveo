SET(CMAKE_C_COMPILER		/opt/nec/ve/bin/nfort)
SET(CMAKE_CXX_COMPILER		/opt/nec/ve/bin/nfort)
SET_DIRECTORY_PROPERTIES	(PROPERTIES COMPILE_OPTIONS "")
SET_DIRECTORY_PROPERTIES	(PROPERTIES LINK_OPTIONS "")
SET_DIRECTORY_PROPERTIES	(PROPERTIES LINK_DIRECTORIES "")

LINK_DIRECTORIES		(/opt/nec/ve/lib)

# AVEORUN ----------------------------------------------------------------------
SET(FILE_AVEORUN ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/aveorun)
SET(FILE_AVEORUN_FTRACE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/aveorun-ftrace)

ADD_EXECUTABLE			(aveorun $<TARGET_OBJECTS:aveorun_objs>)
TARGET_LINK_OPTIONS		(aveorun PRIVATE -O0 -g -pthread -fopenmp -v -cxxlib)
TARGET_LINK_LIBRARIES	(aveorun aveoVE veio)

ADD_EXECUTABLE			(aveorun_ftrace $<TARGET_OBJECTS:aveorun_ftrace_objs>)
TARGET_LINK_OPTIONS		(aveorun_ftrace PRIVATE -O0 -g -pthread -ftrace -fopenmp -cxxlib)
TARGET_LINK_LIBRARIES	(aveorun_ftrace aveoVE veio dl veftrace)

# Install ----------------------------------------------------------------------
INSTALL(TARGETS aveorun aveorun_ftrace RUNTIME DESTINATION libexec)