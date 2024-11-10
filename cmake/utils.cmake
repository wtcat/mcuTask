#=========================
# CMake Helper Function
#=========================

# Common interface library
add_library(common_interface INTERFACE )

function(include_dirs)
	foreach(arg ${ARGV})
		if(IS_ABSOLUTE ${arg})
			set(path ${arg})
		else()
			set(path ${CMAKE_CURRENT_SOURCE_DIR}/${arg})
		endif()
		target_include_directories(common_interface INTERFACE ${path})
	endforeach()
endfunction()

function(library_link libname item)
  target_link_libraries(${libname} PUBLIC ${item} ${ARGN})
endfunction()

# Static library
macro(static_library name)
	add_library(${name} STATIC "")
	set_property(GLOBAL APPEND PROPERTY common_interface ${name})
	target_link_libraries(${name} PUBLIC common_interface)
endmacro()

# Shared library
macro(shared_library name)
	add_library(${name} SHARED "")
	set_property(GLOBAL APPEND PROPERTY common_interface ${name})
	target_link_libraries(${name} PUBLIC common_interface)
endmacro()

# Link target
macro(collect_link_libraries name target_name )
	get_property(_libs GLOBAL PROPERTY common_interface)
	foreach(item ${_libs})
	  add_dependencies(${target_name} ${item})
	  message(STATUS "-> ${item}")
	endforeach()
	set(${name} ${_libs})
endmacro()

function(lib_include_directories)
	target_include_directories(common_interface INTERFACE ${ARGV})
endfunction()

function(compile_options)
  target_compile_options(common_interface INTERFACE ${ARGV})
endfunction()

function(add_subdirectory_ifdef feature_toggle)
  if(${${feature_toggle}})
    add_subdirectory(${ARGN})
  endif()
endfunction()

function(add_subdirectory_ifndef feature_toggle)
  if(${${feature_toggle}})
    add_subdirectory(${ARGN})
  endif()
endfunction()

function(add_subdirectory_ifndef feature_toggle source_dir)
  if(NOT ${feature_toggle})
    add_subdirectory(${source_dir} ${ARGN})
  endif()
endfunction()
