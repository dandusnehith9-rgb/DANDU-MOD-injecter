cmake_minimum_required(VERSION 3.16)
project(DANDU_MOD_injecter VERSION 1.0 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 COMPONENTS Core Gui Widgets QUIET)
if (Qt6_FOUND)
    set(QT_PACKAGE Qt6)
else()
    find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)
    set(QT_PACKAGE Qt5)
endif()

find_package(ZLIB QUIET)
if (ZLIB_FOUND)
    set(ZLIB_LINK_TARGET ZLIB::ZLIB)
else()
    find_path(DANDU_ZLIB_INCLUDE_DIR
        NAMES zlib.h
        PATHS
            "C:/Qt/Tools/mingw1310_64/x86_64-w64-mingw32/include"
            "C:/Qt/6.11.1/mingw_64/include/QtZlib"
    )
    find_library(DANDU_ZLIB_LIBRARY
        NAMES z zlib zlib1
        PATHS
            "C:/Qt/Tools/mingw1310_64/x86_64-w64-mingw32/lib"
    )

    if (DANDU_ZLIB_INCLUDE_DIR AND DANDU_ZLIB_LIBRARY)
        add_library(DANDU_ZLIB UNKNOWN IMPORTED)
        set_target_properties(DANDU_ZLIB PROPERTIES
            IMPORTED_LOCATION "${DANDU_ZLIB_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${DANDU_ZLIB_INCLUDE_DIR}"
        )
        set(ZLIB_LINK_TARGET DANDU_ZLIB)
    else()
        message(FATAL_ERROR "ZLIB not found. Install zlib development files or set ZLIB_INCLUDE_DIR/ZLIB_LIBRARY.")
    endif()
endif()

add_executable(DANDU_MOD_injecter
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/DropCard.h
    src/DropCard.cpp
    src/ModTypes.h
    src/SimpleZipReader.h
    src/SimpleZipReader.cpp
    src/VersionMatcher.h
    src/VersionMatcher.cpp
    src/ModAnalyzer.h
    src/ModAnalyzer.cpp
    src/MinecraftManager.h
    src/MinecraftManager.cpp
)

target_link_libraries(DANDU_MOD_injecter
    PRIVATE
    ${QT_PACKAGE}::Core
    ${QT_PACKAGE}::Gui
    ${QT_PACKAGE}::Widgets
    ${ZLIB_LINK_TARGET}
)

if (WIN32)
    set_target_properties(DANDU_MOD_injecter PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
