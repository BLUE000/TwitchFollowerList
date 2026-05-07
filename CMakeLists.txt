cmake_minimum_required(VERSION 3.16)

project(FollowerList VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network LinguistTools)

# ソースファイルの定義
file(GLOB_RECURSE SOURCES 
    "main.cpp"
    "core/*.cpp"
    "core/*.h"
    "api/*.cpp"
    "api/*.h"
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

install(TARGETS FollowerList
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
