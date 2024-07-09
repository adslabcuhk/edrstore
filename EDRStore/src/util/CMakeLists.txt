#src/util
aux_source_directory(. UTIL_SRC)
add_library(UTIL_LIB ${UTIL_SRC})
target_link_libraries(UTIL_LIB ${LINK_OBJ})