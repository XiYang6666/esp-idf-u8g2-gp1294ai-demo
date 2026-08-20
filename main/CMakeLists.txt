idf_component_register(
        SRCS
        "main.cpp"
        INCLUDE_DIRS
        "."
)
#target_compile_definitions(${COMPONENT_LIB} PRIVATE U8X8_WITH_USER_PTR=1)