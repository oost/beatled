
include("link_threads_if_necessary.cmake")
include("link_atomic_if_necessary.cmake")

SET( RESTINIO_STAND_ALONE_ASIO_HEADERS ${CMAKE_CURRENT_LIST_DIR}/../external/asio/asio/include )
SET( RESTINIO_STAND_ALONE_ASIO_DEFINES -DASIO_STANDALONE -DASIO_HAS_STD_CHRONO -DASIO_DISABLE_STD_STRING_VIEW)
set(RESTINIO_USE_BOOST_ASIO "none")

add_subdirectory(http_parser)
# add_subdirectory(fmt)
add_subdirectory(restinio)
# add_subdirectory(json)
# add_subdirectory(clara)
# add_subdirectory(asio)

