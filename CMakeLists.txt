# $Id$
#
# (C) Copyright 2011 Johns Hopkins University (JHU), All Rights
# Reserved.
#
# --- begin cisst license - do not edit ---
#
# This software is provided "as is" under an open source license, with
# no warranty.  The complete license can be found in license.txt and
# http://www.cisst.org/cisst/license.txt.
#
# --- end cisst license ---

set (REQUIRED_CISST_LIBRARIES cisstCommon cisstOSAbstraction cisstVector cisstVectorQt cisstMultiTask cisstStereoVision cisstStereoVisionQt cisstQt)

# find cisst and make sure the required libraries have been compiled
find_package (cisst REQUIRED ${REQUIRED_CISST_LIBRARIES} QUIET)

if (cisst_FOUND_AS_REQUIRED)

  # load cisst configuration
  include (${CISST_USE_FILE})

  include_directories (${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

  qt4_wrap_cpp (QT_WRAPPED_CPP
                sdpPlayerBase.h
                sdpPlayerExample.h
                sdpPlayerManager.h
                sdpPlayerVideo.h
                sdpPlayerPlot2D.h
                )

  qt4_wrap_ui (QT_WRAPPED_UI
               sdpPlayerManagerWidget.ui
               sdpPlayerWidget.ui)

  add_library (sawDataPlayerComponents ${IS_SHARED}
               ${QT_WRAPPED_CPP}
               ${QT_WRAPPED_UI}
               sdpExport.h
               sdpPlayerBase.h
               sdpPlayerBase.cpp
               sdpPlayerManager.h
               sdpPlayerManager.cpp
               sdpSaveParameters.h
               sdpSaveParameters.cpp
               sdpPlayerDataInfo.h
               sdpPlayerDataInfo.cpp
               sdpPlayerExample.h
               sdpPlayerExample.cpp
               sdpPlayerPlot2D.h
               sdpPlayerPlot2D.cpp
               sdpPlayerVideo.h
               sdpPlayerVideo.cpp
               sdpPlayerParseStateTableData.h
               sdpPlayerParseStateTableData.cpp
               )

  cisst_target_link_libraries (sawDataPlayerComponents ${REQUIRED_CISST_LIBRARIES})

  add_executable (sawDataPlayer
                  main.cpp)

  target_link_libraries (sawDataPlayer sawDataPlayerComponents)
  cisst_target_link_libraries (sawDataPlayer ${REQUIRED_CISST_LIBRARIES})

  add_executable (sawQtVideoPlayer
                  qtVideoPlayer.cpp)

  target_link_libraries (sawQtVideoPlayer sawDataPlayerComponents)
  cisst_target_link_libraries (sawQtVideoPlayer ${REQUIRED_CISST_LIBRARIES})

else (cisst_FOUND_AS_REQUIRED)
  cisst_information_message_missing_libraries (${REQUIRED_CISST_LIBRARIES})
endif (cisst_FOUND_AS_REQUIRED)
