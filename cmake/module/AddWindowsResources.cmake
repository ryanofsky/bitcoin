# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include_guard(GLOBAL)

function(add_windows_resources target rc_file)
  if(WIN32)
    target_sources(${target} PRIVATE ${rc_file})
  endif()
endfunction()

# Add version/icon resources and a fusion manifest to a GUI executable.
# RC_TARGET is set to the target name; RC_DESCRIPTION is a short description string.
function(add_windows_gui_resources target description)
  if(WIN32)
    set(RC_TARGET "${target}")
    set(RC_DESCRIPTION "${description}")
    configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/res/bitcoin-gui-res.rc.in
      ${CMAKE_CURRENT_BINARY_DIR}/${target}-res.rc
      @ONLY
    )
    add_windows_resources(${target} ${CMAKE_CURRENT_BINARY_DIR}/${target}-res.rc)
    add_windows_application_manifest(${target})
  endif()
endfunction()

# Add a fusion manifest to Windows executables.
# See: https://learn.microsoft.com/en-us/windows/win32/sbscs/application-manifests
function(add_windows_application_manifest target)
  if(WIN32)
    configure_file(${PROJECT_SOURCE_DIR}/cmake/windows-app.manifest.in ${target}.manifest USE_SOURCE_PERMISSIONS)
    file(CONFIGURE
      OUTPUT ${target}-manifest.rc
      CONTENT "1 /* CREATEPROCESS_MANIFEST_RESOURCE_ID */ 24 /* RT_MANIFEST */ \"${target}.manifest\""
    )
    add_windows_resources(${target} ${CMAKE_CURRENT_BINARY_DIR}/${target}-manifest.rc)
  endif()
endfunction()
