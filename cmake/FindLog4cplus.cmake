# Locate Log4cplus library
# This module defines
# LOG4CPLUS_FOUND, if false, do not try to link to Log4cplus
# LOG4CPLUS_LIBRARIES
# LOG4CPLUS_INCLUDE_DIR, where to find log4cplus.hpp

FIND_PATH(LOG4CPLUS_INCLUDE_DIR log4cplus/logger.h
  HINTS
  $ENV{LOG4CPLUS_DIR}
  PATH_SUFFIXES include
  PATHS
  /usr/local
  /usr
  /opt
)

FIND_LIBRARY(LOG4CPLUS_RELEASE_LIBRARY
  NAMES LOG4CPLUS log4cplus
  PATHS
  /usr/local/lib
  /usr/lib
)


IF(LOG4CPLUS_RELEASE_LIBRARY)
  SET(LOG4CPLUS_LIBRARIES  ${LOG4CPLUS_RELEASE_LIBRARY})
ENDIF()

IF(Y)
	SET(LOG4CPLUS_FOUND TRUE)
ENDIF()

IF (LOG4CPLUS_FOUND)
   IF (NOT LOG4CPLUS_FIND_QUIETLY)
      MESSAGE(STATUS "Found Log4cplus: ${LOG4CPLUS_LIBRARIES}")
   ENDIF(NOT LOG4CPLUS_FIND_QUIETLY)
ENDIF()

MARK_AS_ADVANCED(LOG4CPLUS_INCLUDE_DIR LOG4CPLUS_LIBRARIES LOG4CPLUS_DEBUG_LIBRARY LOG4CPLUS_RELEASE_LIBRARY)