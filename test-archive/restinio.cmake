
set(RESTINIO_INSTALL OFF)
set(RESTINIO_TEST OFF)
set(RESTINIO_SAMPLE OFF)
set(RESTINIO_INSTALL_SAMPLES OFF)
set(RESTINIO_BENCH OFF)
set(RESTINIO_INSTALL_BENCHES OFF)

set(RESTINIO_DIR ${CMAKE_CURRENT_LIST_DIR}/lib/restinio-0.6.16/dev/)

option(RESTINIO_INSTALL "Generate the install target." OFF)
option(RESTINIO_FMT_HEADER_ONLY "Include fmt library as header-only." ON)

SET(RESTINIO_USE_BOOST_ASIO "none" CACHE STRING "Use boost version of ASIO")



list(APPEND CMAKE_MODULE_PATH "${RESTINIO_DIR}/cmake/modules")
include("${RESTINIO_DIR}/cmake/link_threads_if_necessary.cmake")
include("${RESTINIO_DIR}/cmake/link_atomic_if_necessary.cmake")
SET( RESTINIO_STAND_ALONE_ASIO_HEADERS ${RESTINIO_DIR}/asio/include )
SET( RESTINIO_STAND_ALONE_ASIO_DEFINES -DASIO_STANDALONE -DASIO_HAS_STD_CHRONO -DASIO_DISABLE_STD_STRING_VIEW)

include_directories("${RESTINIO_DIR}/clara")
add_subdirectory(${RESTINIO_DIR}/nodejs/http_parser)
add_subdirectory(${RESTINIO_DIR}/fmt)
add_subdirectory(${RESTINIO_DIR}/restinio)
