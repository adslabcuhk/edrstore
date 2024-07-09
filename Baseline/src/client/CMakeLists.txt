#src/CLIENT_OBJ
aux_source_directory(. CLIENT_SRC)
add_library(CLIENT_LIB ${CLIENT_SRC})
target_link_libraries(CLIENT_LIB ${LINK_OBJ})