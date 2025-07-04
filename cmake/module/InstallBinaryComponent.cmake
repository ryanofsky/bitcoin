# Copyright (c) 2025-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include_guard(GLOBAL)
include(GNUInstallDirs)

function(install_binary_component component)
  cmake_parse_arguments(PARSE_ARGV 1
    IC                # prefix
    "HAS_MANPAGE;INTERNAL" # options
    ""                # one_value_keywords
    ""                # multi_value_keywords
  )
  set(target_name ${component})
  if(IC_INTERNAL)
    # Install in libexec/ instead of bin/
    set(runtime_dest ${CMAKE_INSTALL_LIBEXECDIR})
    # Build in libexec/ instead of bin/ so build layout matches install layout.
    set_target_properties(${target_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${runtime_dest}")
    # Create symlink from build/bin/ to build/libexec/ for convenience.
    set(filename "${CMAKE_EXECUTABLE_PREFIX}${target_name}${CMAKE_EXECUTABLE_SUFFIX}")
    add_custom_target(${target_name}_bin_symlink
        COMMAND ${CMAKE_COMMAND} -E create_symlink "${PROJECT_BINARY_DIR}/${runtime_dest}/${filename}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${filename}"
        VERBATIM
    )
    add_dependencies(${target_name} ${target_name}_bin_symlink)
  else()
    set(runtime_dest ${CMAKE_INSTALL_BINDIR})
  endif()
  install(TARGETS ${target_name}
    RUNTIME DESTINATION ${runtime_dest}
    COMPONENT ${component}
  )
  if(INSTALL_MAN AND IC_HAS_MANPAGE)
    install(FILES ${PROJECT_SOURCE_DIR}/doc/man/${target_name}.1
      DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
      COMPONENT ${component}
    )
  endif()
endfunction()
