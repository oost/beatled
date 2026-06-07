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

# Wall-clock build time, stamped in two complementary forms so the
# firmware can both report a human-readable string in logs and a numeric
# microsecond timestamp on the wire (HELLO_REQUEST → server → React UI).
string(TIMESTAMP BUILD_TIME_HUMAN "%Y-%m-%dT%H:%M:%SZ" UTC)
string(TIMESTAMP BUILD_TIME_S "%s" UTC)
# Compose us-since-epoch as <s>000000ULL (literal concatenation avoids
# 32-bit overflow inside CMake's math expression evaluator).
set(BUILD_TIME_US "${BUILD_TIME_S}000000ULL")

set(NEW_CONTENT "#pragma once\n")
set(NEW_CONTENT "${NEW_CONTENT}#define BEATLED_GIT_HASH \"${GIT_HASH}\"\n")
set(NEW_CONTENT "${NEW_CONTENT}#define BEATLED_BUILD_TIME \"${BUILD_TIME_HUMAN}\"\n")
set(NEW_CONTENT "${NEW_CONTENT}#define BEATLED_BUILD_TIME_US ${BUILD_TIME_US}\n")

if(EXISTS "${OUT}")
  file(READ "${OUT}" OLD_CONTENT)
else()
  set(OLD_CONTENT "")
endif()

if(NOT "${OLD_CONTENT}" STREQUAL "${NEW_CONTENT}")
  file(WRITE "${OUT}" "${NEW_CONTENT}")
endif()
