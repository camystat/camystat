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

# Copy runtime assets (best effort, mirrors VS "CopyFileToFolders")
add_custom_command(TARGET camystat POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:camystat>/licenses"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${CMAKE_CURRENT_SOURCE_DIR}/citeme.txt"
          "$<TARGET_FILE_DIR:camystat>/citeme.txt"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${CMAKE_CURRENT_SOURCE_DIR}/icon.ico"
          "$<TARGET_FILE_DIR:camystat>/icon.ico"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${CMAKE_CURRENT_SOURCE_DIR}/plot.exe"
          "$<TARGET_FILE_DIR:camystat>/plot.exe"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${CMAKE_CURRENT_SOURCE_DIR}/misc/licenses"
          "$<TARGET_FILE_DIR:camystat>/licenses"
  VERBATIM
)

