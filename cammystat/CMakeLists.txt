cmake_minimum_required(VERSION 3.20)

add_executable(cammystat
  Application.cpp
  Cammystat.cpp
  FsUtils.cpp
  IterativeReportWriter.cpp
  MainFrame.cpp
  MiscUtils.cpp
  Savgol.cpp
  Utils.cpp
)

target_include_directories(cammystat
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../eigen"
)

# wxWidgets
find_package(wxWidgets REQUIRED COMPONENTS core base)
target_link_libraries(cammystat PRIVATE ${wxWidgets_LIBRARIES})
target_include_directories(cammystat PRIVATE ${wxWidgets_INCLUDE_DIRS})
target_compile_definitions(cammystat PRIVATE ${wxWidgets_DEFINITIONS})

# On Linux, copy wxWidgets .so files into the build dir so the executable finds them at runtime
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  list(LENGTH wxWidgets_LIBRARIES wx_lib_count)
  if(wx_lib_count GREATER 0)
    list(GET wxWidgets_LIBRARIES 0 wx_first_lib)
    get_filename_component(wx_lib_dir "${wx_first_lib}" DIRECTORY)
    file(GLOB wx_so_files "${wx_lib_dir}/libwx_*.so*")
    foreach(wx_so ${wx_so_files})
      add_custom_command(TARGET cammystat POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${wx_so}" "$<TARGET_FILE_DIR:cammystat>/"
        VERBATIM
      )
    endforeach()
  endif()
endif()

# OpenCV
find_package(OpenCV REQUIRED)
target_link_libraries(cammystat PRIVATE ${OpenCV_LIBS})
target_include_directories(cammystat PRIVATE ${OpenCV_INCLUDE_DIRS})

# Copy runtime assets (best effort, mirrors VS "CopyFileToFolders")
add_custom_command(TARGET cammystat POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:cammystat>/licenses"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${CMAKE_CURRENT_SOURCE_DIR}/citeme.txt"
          "$<TARGET_FILE_DIR:cammystat>/citeme.txt"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${CMAKE_CURRENT_SOURCE_DIR}/icon.ico"
          "$<TARGET_FILE_DIR:cammystat>/icon.ico"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${CMAKE_CURRENT_SOURCE_DIR}/plot.exe"
          "$<TARGET_FILE_DIR:cammystat>/plot.exe"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${CMAKE_CURRENT_SOURCE_DIR}/misc/licenses"
          "$<TARGET_FILE_DIR:cammystat>/licenses"
  VERBATIM
)

