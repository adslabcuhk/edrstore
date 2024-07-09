aux_source_directory(. KEY_MANAGER_SRC)
add_library(KM_LIB ${KEY_MANAGER_SRC})
target_link_libraries(KM_LIB ${LINK_OBJ})