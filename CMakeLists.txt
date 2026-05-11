cmake_minimum_required(VERSION 3.16)

project(FollowerList VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network LinguistTools Test)

# クライアントIDの読み込み (Git管理外のファイルから取得)
set(CLIENT_ID_FILE "${CMAKE_CURRENT_SOURCE_DIR}/TWITCH_CLIENT_ID.txt")
if(EXISTS "${CLIENT_ID_FILE}")
    file(READ "${CLIENT_ID_FILE}" TWITCH_CLIENT_ID_VAL)
    string(STRIP "${TWITCH_CLIENT_ID_VAL}" TWITCH_CLIENT_ID_VAL)
else()
    set(TWITCH_CLIENT_ID_VAL "REPLACE_ME")
    message(WARNING "TWITCH_CLIENT_ID.txt not found. Using placeholder.")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Test Configuration ---
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/refs/heads/main.zip
    )
    # For Windows: prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    enable_testing()
endif()

# ソースファイルの定義
file(GLOB_RECURSE SOURCES 
    api/twitch_authenticator.h api/twitch_authenticator.cpp
    api/twitch_api_client.h api/twitch_api_client.cpp
    api/file_manager.h api/file_manager.cpp
    api/logger.h api/logger.cpp
    "main.cpp"
    "core/*.cpp"
    "core/*.h"
    "ui/*.cpp"
    "ui/*.h"
    "ui/*.ui"
)

add_executable(FollowerList ${SOURCES})

# 翻訳ファイルの読み込み設定
set(TS_FILES FollowerList_ja_JP.ts)
qt6_add_translations(FollowerList TS_FILES ${TS_FILES})

# インクルードパスの設定
# TransCipher は Lib フォルダ内の公開ヘッダのみを参照するように変更
target_include_directories(FollowerList PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Lib/TransCipher/include
)

target_link_libraries(FollowerList PRIVATE 
    Qt6::Widgets 
    Qt6::Network
    "${CMAKE_CURRENT_SOURCE_DIR}/Lib/TransCipher/lib/libTransCipher.dll.a"
)

# コンパイル定義にクライアントIDを注入
target_compile_definitions(FollowerList PRIVATE TWITCH_CLIENT_ID="${TWITCH_CLIENT_ID_VAL}")

# 実行ファイルと同じディレクトリに DLL をコピーする設定
add_custom_command(TARGET FollowerList POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/Lib/TransCipher/lib/libTransCipher.dll"
        $<TARGET_FILE_DIR:FollowerList>
)

if(WIN32)
    set_target_properties(FollowerList PROPERTIES
        WIN32_EXECUTABLE ON
    )
endif()

if(BUILD_TESTS)
    add_executable(unit_tests
        tests/test_main.cpp
        tests/test_file_manager.cpp
        api/file_manager.cpp
        api/logger.cpp
    )
    target_compile_definitions(unit_tests PRIVATE TWITCH_CLIENT_ID="${TWITCH_CLIENT_ID_VAL}")
    target_include_directories(unit_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/Lib/TransCipher/include
    )
    target_link_libraries(unit_tests PRIVATE 
        GTest::gtest_main
        Qt6::Network
        "${CMAKE_CURRENT_SOURCE_DIR}/Lib/TransCipher/lib/libTransCipher.dll.a"
    )
    add_test(NAME AllTests COMMAND unit_tests)
    
    # Copy DLL for test execution
    add_custom_command(TARGET unit_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/Lib/TransCipher/lib/libTransCipher.dll"
            $<TARGET_FILE_DIR:unit_tests>
    )

    add_executable(integration_tests
        tests/test_integration.cpp
        core/app_controller.cpp
        api/file_manager.cpp
        api/logger.cpp
        api/twitch_api_client.cpp
        api/twitch_authenticator.cpp
        ui/mainwindow.cpp
    )
    target_compile_definitions(integration_tests PRIVATE TWITCH_CLIENT_ID="${TWITCH_CLIENT_ID_VAL}")
    target_include_directories(integration_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/Lib/TransCipher/include
    )
    target_link_libraries(integration_tests PRIVATE 
        Qt6::Test
        Qt6::Widgets
        Qt6::Network
        "${CMAKE_CURRENT_SOURCE_DIR}/Lib/TransCipher/lib/libTransCipher.dll.a"
    )
    add_test(NAME IntegrationTests COMMAND integration_tests)

    add_custom_command(TARGET integration_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/Lib/TransCipher/lib/libTransCipher.dll"
            $<TARGET_FILE_DIR:integration_tests>
    )
endif()
