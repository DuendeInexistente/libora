

ADD_LIBRARY(ora SHARED ora.c internal.c zip/unzip.c zip/zip.c zip/ioapi.c layers/layers.c layers/stack.c layers/stack_parser.c utils/stringbuffer.c)

SET(HEADER_FILES ora.h)

IF (UNIX)
INSTALL(TARGETS ora LIBRARY DESTINATION lib)
INSTALL(FILES ${HEADER_FILES} DESTINATION include)
ENDIF (UNIX)


