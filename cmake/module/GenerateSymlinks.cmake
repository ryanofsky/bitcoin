# Function: create_symlink_for_target
# -----------------------------------
# Creates a symlink for a given CMake target so it can be built in
# CMAKE_RUNTIME_OUTPUT_DIRECTORY but remains accessible in its default subdirectory
# location.
#
# Parameters:
# - target_name: The name of the CMake target (must be defined via `add_executable`
#   or `add_library`).
#
# Example:
#   create_symlink_for_target(my_executable)
#   - If my_executable previously resided in `CMAKE_CURRENT_BINARY_DIR/my_executable`,
#     but is now in `CMAKE_RUNTIME_OUTPUT_DIRECTORY/my_executable`, a symlink will be created in
#     the original location pointing to the new location.
#
# This function is intended to be called automatically for all targets in a project.

function(create_symlink_for_target target_name)
    get_target_property(target_type ${target_name} TYPE)
    get_target_property(target_old_path ${target_name} BINARY_DIR)
    if(target_type STREQUAL "EXECUTABLE")
        set(target_new_path "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
        set(target_output_name "${CMAKE_EXECUTABLE_PREFIX}${target_name}${CMAKE_EXECUTABLE_SUFFIX}")
    else()
        return()
    endif()

    add_custom_target(${target_name}_create_symlink
        COMMAND ${CMAKE_COMMAND} -E create_symlink "${target_new_path}/${target_output_name}" "${target_old_path}/${target_output_name}"
        COMMENT "Creating symlink from ${target_old_path} to ${target_new_path}"
        VERBATIM
    )
    add_dependencies(${target_name} ${target_name}_create_symlink)
endfunction()

# Function: get_all_targets_recursive
# -----------------------------------
# Recursively retrieves all CMake targets from the given directory and its subdirectories.
#
# This function is useful when collecting all targets from a multi-directory CMake project.
# It ensures that targets from all nested subdirectories are included.
#
# Parameters:
# - out_var: The variable to store the collected target names.
# - dir: The starting directory for recursion (typically CMAKE_SOURCE_DIR).
#
# Example:
#   get_all_targets_recursive(all_targets ${CMAKE_SOURCE_DIR})
#   - Populates `all_targets` with all executable and library targets defined in the project.

function(get_all_targets_recursive out_var dir)
    # Get targets in the current directory
    get_property(local_targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)

    # Get subdirectories
    get_property(subdirs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)

    # Recursively collect targets from subdirectories
    foreach(subdir IN LISTS subdirs)
        get_all_targets_recursive(subdir_targets "${subdir}")
        list(APPEND local_targets ${subdir_targets})
    endforeach()

    set(${out_var} ${local_targets} PARENT_SCOPE)
endfunction()

# Function: create_symlinks_for_all_targets
# -----------------------------------------
# Iterates over all CMake targets and applies `create_symlink_for_target` to ensure
# compatibility with the new centralized output directory structure.
#
# This function retrieves all defined targets within the current directory scope
# and ensures that any executable or library receives a compatibility symlink.
#
# - Calls `get_property(all_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)`
#   to retrieve the list of all targets.
# - Iterates through each target and invokes `create_symlink_for_target()`.
#
# This function should be invoked once at the end of the CMake configuration
# to apply symlink creation to all relevant targets in the project.

function(create_symlinks_for_all_targets)
    get_all_targets_recursive(all_targets ${CMAKE_SOURCE_DIR})  # Start from the root directory
    foreach(target IN LISTS all_targets)
        create_symlink_for_target(${target})
    endforeach()
endfunction()
