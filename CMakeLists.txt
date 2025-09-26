cmake_minimum_required(VERSION 3.16)

project(ogrenci_yoklama_kontrol VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find an installed Qt 6
find_package(Qt6 COMPONENTS Core Gui Widgets Sql SerialPort Multimedia REQUIRED)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        databasemanager.cpp
        databasemanager.h
        create_sqlite_db.cpp
        sqlite3.c
        loginwidget.cpp
        loginwidget.h
        loginwidget.ui
        adminwidget.cpp
        adminwidget.h
        adminwidget.ui
        teacherwidget.cpp
        teacherwidget.h
        teacherwidget.ui
        tablehelper.cpp
        tablehelper.h
        studentwidget.cpp
        studentwidget.h
        studentwidget.ui
)

set(UI_FILES
    mainwindow.ui
    loginwidget.ui
    adminwidget.ui
    teacherwidget.ui
    studentwidget.ui
)

add_executable(${PROJECT_NAME}
    ${PROJECT_SOURCES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
    Qt6::SerialPort
    Qt6::Multimedia
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_TYPE GUI
)
