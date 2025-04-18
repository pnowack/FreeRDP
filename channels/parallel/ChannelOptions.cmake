set(OPTION_DEFAULT ON)
set(OPTION_CLIENT_DEFAULT ON)
set(OPTION_SERVER_DEFAULT OFF)

if(WIN32)
  set(OPTION_CLIENT_DEFAULT OFF)
  set(OPTION_SERVER_DEFAULT OFF)
  message("Serial redirection not supported on windows")
endif()

if(APPLE)
  set(OPTION_CLIENT_DEFAULT OFF)
  set(OPTION_SERVER_DEFAULT OFF)
  message("Serial redirection not supported on apple")
endif()

if(ANDROID)
  set(OPTION_CLIENT_DEFAULT OFF)
  set(OPTION_SERVER_DEFAULT OFF)
  message("Serial redirection not supported on android")
endif()

define_channel_options(
  NAME
  "parallel"
  TYPE
  "device"
  DESCRIPTION
  "Parallel Port Virtual Channel Extension"
  SPECIFICATIONS
  "[MS-RDPESP]"
  DEFAULT
  ${OPTION_DEFAULT}
  CLIENT_DEFAULT
  ${OPTION_CLIENT_DEFAULT}
  SERVER_DEFAULT
  ${OPTION_SERVER_DEFAULT}
)
