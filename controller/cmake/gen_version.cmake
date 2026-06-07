execute_process(
  COMMAND git -C "${BEATLED_SRC_DIR}" rev-parse --short HEAD
  OUTPUT_VARIABLE GIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
)
execute_process(
  COMMAND git -C "${BEATLED_SRC_DIR}" status --porcelain
  OUTPUT_VARIABLE GIT_DIRTY
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
)
if(NOT GIT_HASH)
  set(GIT_HASH "unknown")
endif()
if(GIT_DIRTY)
  set(GIT_HASH "${GIT_HASH}-dirty")
endif()

set(NEW_CONTENT "#pragma once\n#define BEATLED_GIT_HASH \"${GIT_HASH}\"\n")

if(EXISTS "${OUT}")
  file(READ "${OUT}" OLD_CONTENT)
else()
  set(OLD_CONTENT "")
endif()

if(NOT "${OLD_CONTENT}" STREQUAL "${NEW_CONTENT}")
  file(WRITE "${OUT}" "${NEW_CONTENT}")
endif()
