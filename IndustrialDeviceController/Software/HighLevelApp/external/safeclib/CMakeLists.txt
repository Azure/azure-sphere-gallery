CMAKE_MINIMUM_REQUIRED(VERSION 3.8)

aux_source_directory(. SAFECLIB_SRCS)
add_compile_options(-Wall -Wmissing-prototypes -Wno-unused-but-set-variable -Wno-unused-variable -Wno-sign-conversion)


# Create library
add_library(safeclib STATIC ${SAFECLIB_SRCS})
