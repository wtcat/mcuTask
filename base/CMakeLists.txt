# C++ base library

if (CONFIG_CPLUSPLUS)
static_library(ccbase)

target_sources(ccbase
    PRIVATE
    containers/linked_list.cc
)

endif(CONFIG_CPLUSPLUS)
