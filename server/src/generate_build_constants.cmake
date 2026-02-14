string(TIMESTAMP RPIZ_BS_BUILDTIME "%Y-%m-%d %H:%M:%S")
set(RPIZBeatServer_VERSION_MAJOR ${RPIZ_BS_VERSION_MAJOR})
set(RPIZBeatServer_VERSION_MINOR ${RPIZ_BS_VERSION_MINOR})

# Always write the file (configure_file skips if content unchanged).
# We write via file(WRITE) to ensure mtime updates on every build,
# which forces recompilation of files that include this header.
file(READ "${INPUT_FILE}" TEMPLATE_CONTENT)
string(REPLACE "@RPIZ_BS_BUILDTIME@" "${RPIZ_BS_BUILDTIME}" TEMPLATE_CONTENT "${TEMPLATE_CONTENT}")
string(REPLACE "\${RPIZBeatServer_VERSION_MAJOR}" "${RPIZBeatServer_VERSION_MAJOR}" TEMPLATE_CONTENT "${TEMPLATE_CONTENT}")
string(REPLACE "\${RPIZBeatServer_VERSION_MINOR}" "${RPIZBeatServer_VERSION_MINOR}" TEMPLATE_CONTENT "${TEMPLATE_CONTENT}")
file(WRITE "${OUTPUT_FILE}" "${TEMPLATE_CONTENT}")
