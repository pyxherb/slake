aux_source_directory("." VIZARD_SRCS)

add_library(vzc STATIC ${VIZARD_SRCS})
