cmake_minimum_required(VERSION 3.20)

add_executable(camystat
  Application.cpp
  Camystat.cpp
  FsUtils.cpp
  IterativeReportWriter.cpp
  MainFrame.cpp
  MiscUtils.cpp
  Savgol.cpp
  Utils.cpp
)

if(APPLE)
  set_target_properties(camystat PROPERTIES
    OUTPUT_NAME Camystat
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.camystat.camystat"
    MACOSX_BUNDLE_BUNDLE_NAME "Camystat"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_COPYRIGHT "Camystat contributors"
  )
endif()

target_include_directories(camystat
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../eigen"
)

# wxWidgets
find_package(wxWidgets REQUIRED COMPONENTS core base)
target_link_libraries(camystat PRIVATE ${wxWidgets_LIBRARIES})
target_include_directories(camystat PRIVATE ${wxWidgets_INCLUDE_DIRS})
target_compile_definitions(camystat PRIVATE ${wxWidgets_DEFINITIONS})

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        list(LENGTH wxWidgets_LIBRARIES wx_lib_count)
        if(wx_lib_count GREATER 0)
                list(GET wxWidgets_LIBRARIES 0 FIRST_LIB_PATH)
                string(REGEX REPLACE "^-L" "" WX_LIB_DIR "${FIRST_LIB_PATH}")
                string(REGEX REPLACE ";.*" "" WX_LIB_DIR "${WX_LIB_DIR}")

                message(STATUS "wxWidgets library dir detected: ${WX_LIB_DIR}")
                file(GLOB wx_so_files "${WX_LIB_DIR}/libwx_*")
                foreach(wx_so ${wx_so_files})
                        add_custom_command(TARGET camystat POST_BUILD
                                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${wx_so}" "$<TARGET_FILE_DIR:camystat>/"
                                VERBATIM
                        )
                endforeach()
        endif()
endif()

# OpenCV
find_package(OpenCV REQUIRED)
target_link_libraries(camystat PRIVATE ${OpenCV_LIBS})
target_include_directories(camystat PRIVATE ${OpenCV_INCLUDE_DIRS})

if(APPLE)
  # Use Homebrew/system dylibs at runtime (not embedded in the .app).
  set(_camystat_rpaths "/opt/homebrew/lib" "/usr/local/lib")
  foreach(_lib_dir IN LISTS wxWidgets_LIBRARIES OpenCV_LIBS)
    if(_lib_dir MATCHES "^-L(.+)$")
      list(APPEND _camystat_rpaths "${CMAKE_MATCH_1}")
    elseif(_lib_dir MATCHES "\\.dylib$")
      get_filename_component(_libdir "${_lib_dir}" DIRECTORY)
      list(APPEND _camystat_rpaths "${_libdir}")
    endif()
  endforeach()
  list(REMOVE_DUPLICATES _camystat_rpaths)
  string(JOIN ";" _camystat_rpath "${_camystat_rpaths}")
  set_target_properties(camystat PROPERTIES
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "${_camystat_rpath}"
    MACOSX_RPATH ON
  )
  message(STATUS "macOS: using system/Homebrew dylibs (RPATH: ${_camystat_rpath})")
endif()

# Copy runtime assets
if(APPLE)
  set(_camystat_runtime_dir "$<TARGET_BUNDLE_CONTENT_DIR:camystat>/Resources")
else()
  set(_camystat_runtime_dir "$<TARGET_FILE_DIR:camystat>")
endif()

set(_camystat_post_build_commands
  COMMAND ${CMAKE_COMMAND} -E make_directory "${_camystat_runtime_dir}/licenses"
)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/citeme.txt")
  list(APPEND _camystat_post_build_commands
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/citeme.txt"
            "${_camystat_runtime_dir}/citeme.txt"
  )
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/icon.ico")
  list(APPEND _camystat_post_build_commands
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/icon.ico"
            "${_camystat_runtime_dir}/icon.ico"
  )
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/misc/ikona.png")
  list(APPEND _camystat_post_build_commands
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/misc/ikona.png"
            "${_camystat_runtime_dir}/ikona.png"
  )
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plot.exe")
  list(APPEND _camystat_post_build_commands
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/plot.exe"
            "${_camystat_runtime_dir}/plot.exe"
  )
endif()
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/misc/licenses")
  list(APPEND _camystat_post_build_commands
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/misc/licenses"
            "${_camystat_runtime_dir}/licenses"
  )
endif()
add_custom_command(TARGET camystat POST_BUILD
  ${_camystat_post_build_commands}
  VERBATIM
)

if(APPLE)
  install(TARGETS camystat BUNDLE DESTINATION ".")
endif()

