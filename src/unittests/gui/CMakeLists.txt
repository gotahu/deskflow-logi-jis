# SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
# SPDX-License-Identifier: MIT

add_subdirectory(config)
add_subdirectory(core)

create_test(
  NAME LoggerTests
  DEPENDS gui
  SOURCE LoggerTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/gui"
)
create_test(
  NAME FileTransferHubTests
  ARGS -o "${CMAKE_CURRENT_BINARY_DIR}/FileTransferHubTests.txt,txt"
  DEPENDS gui
  LIBS base arch mt io
  SOURCE FileTransferHubTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/gui"
)

create_test(
  NAME FileTransferUiTests
  ARGS -o "${CMAKE_CURRENT_BINARY_DIR}/FileTransferUiTests.txt,txt"
  DEPENDS gui
  LIBS base arch mt io
  SOURCE FileTransferUiTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/gui"
)

if(WIN32)
  target_link_libraries(FileTransferHubTests version)
  target_link_libraries(FileTransferUiTests version)
endif()
