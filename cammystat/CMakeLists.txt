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
          "${CMAKE_CURRENT_SOURCE_DIR}/plot.exe"
          "$<TARGET_FILE_DIR:cammystat>/plot.exe"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${CMAKE_CURRENT_SOURCE_DIR}/misc/licenses"
          "$<TARGET_FILE_DIR:cammystat>/licenses"
  VERBATIM
)

