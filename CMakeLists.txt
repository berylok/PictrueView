cmake_minimum_required(VERSION 3.16)
project(PictureView VERSION 1.3.8.1 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找Qt6库 - 添加 LinguistTools
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent LinguistTools)

# 设置自动处理
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 设置源文件
set(SOURCES
    main.cpp
    canvascontrolpanel.cpp
    configmanager.cpp
    imagewidget.cpp
    thumbnailwidget.cpp
)

# 设置头文件
set(HEADERS
    canvascontrolpanel.h
    configmanager.h
    imagewidget.h
    thumbnailwidget.h
)

# 创建可执行文件
add_executable(PictureView ${SOURCES} ${HEADERS})

# 设置资源文件
set(RESOURCE_FILES app.qrc)

# 添加资源
qt_add_resources(PictureView "app_resources"
    PREFIX "/"
    FILES ${RESOURCE_FILES}
)

# 链接Qt6库
target_link_libraries(PictureView
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Concurrent
)

# 包含目录
target_include_directories(PictureView PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# 设置应用程序图标
if(WIN32)
    set(APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/icons/PictureView.ico")

    if(EXISTS "${APP_ICON}")
        message(STATUS "Setting application icon: ${APP_ICON}")

        # 创建RC文件设置图标
        set(RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/app.rc")
        file(WRITE ${RC_FILE} "IDI_ICON1 ICON DISCARDABLE \"${APP_ICON}\"")
        target_sources(PictureView PRIVATE ${RC_FILE})
    else()
        message(WARNING "Icon file not found: ${APP_ICON}")
    endif()
endif()

# 设置Windows可执行文件属性
if(WIN32)
    set_target_properties(PictureView PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# 添加翻译文件支持
set(TS_FILES
    #translations/PictureView_zh_CN.ts
    translations/PictureView_en_US.ts
)

# 检查翻译文件是否存在并添加
foreach(TS_FILE ${TS_FILES})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${TS_FILE}")
        list(APPEND EXISTING_TS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${TS_FILE}")
    else()
        message(WARNING "Translation file not found: ${TS_FILE}")
    endif()
endforeach()

if(EXISTING_TS_FILES)
    # 使用 Qt6 的翻译功能
    qt6_add_translation(QM_FILES ${EXISTING_TS_FILES})

    # 将 QM 文件添加到资源中，确保程序可以访问它们
    qt_add_resources(PictureView "translation_resources"
        PREFIX "/translations"
        BASE ${CMAKE_CURRENT_BINARY_DIR}
        FILES ${QM_FILES}
    )
endif()

# # 编译器优化设置
# if(CMAKE_BUILD_TYPE STREQUAL "Release")
#     target_compile_options(PictureView PRIVATE
#         $<$<CXX_COMPILER_ID:MSVC>:
#             /O2
#         >
#         $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
#             -O2
#             -msse2
#         >
#     )

#     # 为GCC/Clang移除可能产生问题的优化
#     if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
#         target_compile_options(PictureView PRIVATE
#             -fno-fast-math
#         )
#     endif()
# elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
#     target_compile_options(PictureView PRIVATE
#         $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
#             -O1
#             -g
#         >
#         $<$<CXX_COMPILER_ID:MSVC>:
#             /Od
#             /Zi
#         >
#     )
# endif()

# 设置版本和公司信息
set_target_properties(PictureView PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# 可选：安装目标
install(TARGETS PictureView
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)

# 安装翻译文件
if(QM_FILES)
    install(FILES ${QM_FILES}
        DESTINATION ${CMAKE_INSTALL_DATADIR}/translations
    )
endif()
