add_executable(${ProjectName} WIN32 main.cpp event_dispatcher.cpp
 event_driven.cpp win_proc.cpp win_console.cpp win_window.cpp font.cpp
 utils.cpp config.cpp root_app.cpp special_windows.cpp log_window.cpp)

target_link_libraries(${ProjectName} ${Boost_LIBRARIES})

if (MINGW)
  target_link_options(${ProjectName} PRIVATE -municode)
endif()
