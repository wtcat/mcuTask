static_library(app)

target_sources(app
    PRIVATE
    init.cc
)

add_subdirectory(module)
