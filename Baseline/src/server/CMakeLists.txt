#src/SERVER_OBJ
aux_source_directory(. SERVER_SRC)
add_library(SERVER_LIB ${SERVER_SRC})
target_link_libraries(SERVER_LIB ${LINK_OBJ})