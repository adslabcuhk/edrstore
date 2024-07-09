#src/SERVER_OBJ
aux_source_directory(. REDUCTION_SRC)
add_library(REDUCTION_LIB ${REDUCTION_SRC})
target_link_libraries(REDUCTION_LIB xdeltaLib ${LINK_OBJ})