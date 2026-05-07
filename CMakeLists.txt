cmake_minimum_required(VERSION 3.16)

project(FollowerList VERSION 0.1 LANGUAGES CXX)

# Output directories for build artifacts
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin/Debug")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/Release")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network LinguistTools)

set(TS_FILES FollowerList_ja_JP.ts)

set(PROJECT_SOURCES
        main.cpp
        ui/mainwindow.cpp
        ui/mainwindow.h
        ui/mainwindow.ui
        core/data_models.h
        core/app_controller.cpp
        core/app_controller.h
        api/file_manager.cpp
        api/file_manager.h
        api/twitch_authenticator.cpp
        api/twitch_authenticator.h
        api/twitch_api_client.cpp
        api/twitch_api_client.h
        ${TS_FILES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(FollowerList
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET FollowerList APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(FollowerList SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(FollowerList
            ${PROJECT_SOURCES}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_link_libraries(FollowerList PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network)

# --- TransCipher Integration ---
set(TRANSCIPHER_DIR "${CMAKE_SOURCE_DIR}/../TransCipher")
target_include_directories(FollowerList PRIVATE "${TRANSCIPHER_DIR}/api")

# Link to the built DLL (MinGW output)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(FollowerList PRIVATE "${TRANSCIPHER_DIR}/build/libTransCipher.dll.a")
else()
    target_link_libraries(FollowerList PRIVATE "${TRANSCIPHER_DIR}/build/libTransCipher.dll.a")
endif()

# Copy DLL to bin directory after build
add_custom_command(TARGET FollowerList POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${TRANSCIPHER_DIR}/build/libTransCipher.dll"
    "$<TARGET_FILE_DIR:FollowerList>/libTransCipher.dll"
    COMMENT "Copying TransCipher.dll to app directory..."
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.FollowerList)
endif()
set_target_properties(FollowerList PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(WIN32 AND NOT ANDROID)
    # Qtのbinディレクトリを特定してwindeployqtを探す
    if(QT_VERSION_MAJOR EQUAL 6)
        get_target_property(_qmake_path Qt6::qmake IMPORTED_LOCATION)
    else()
        get_target_property(_qmake_path Qt5::qmake IMPORTED_LOCATION)
    endif()
    get_filename_component(_qt_bin_dir "${_qmake_path}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET FollowerList POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
            ARGS $<TARGET_FILE:FollowerList>
            COMMENT "Running windeployqt to copy required DLLs to output directory..."
        )
    endif()
endif()

include(GNUInstallDirs)
install(TARGETS FollowerList
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(FollowerList)
endif()
