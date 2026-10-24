cmake_minimum_required(VERSION 3.21)

project(diffimg VERSION 2.6.0 LANGUAGES CXX)
include(CTest)
option(DIFFIMG_ASAN "Instrument application and bundled comparison code with MSVC AddressSanitizer" OFF)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ui)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTORCC_OPTIONS --format-version 1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
find_package(Qt6 COMPONENTS Widgets REQUIRED)
find_package(Qwt REQUIRED)
add_library(diffimg_core STATIC
    src/ReviewProject.cpp
    src/ReviewProject.h
    src/ReviewWorkbench.cpp
    src/ReviewWorkbench.h
    # Main sources
    src/AboutDialog.cpp
    src/AboutDialog.h
    src/AppSettings.cpp
    src/AppSettings.h
    src/ChannelCurve.cpp
    src/ChannelCurve.h
    src/DiffImgWindow.cpp
    src/DiffImgWindow.h
    src/FilesDialog.cpp
    src/FilesDialog.h
    src/FilesManager.cpp
    src/FilesManager.h
    src/ImageNavigator.cpp
    src/ImageNavigator.h
    src/ImageView.cpp
    src/ImageView.h
    src/IMessageHandler.h
    src/LogHandler.cpp
    src/LogHandler.h
    src/MiscFunctions.cpp
    src/MiscFunctions.h
    src/PropertyWidget.cpp
    src/PropertyWidget.h
    src/WipeItem.cpp
    src/WipeItem.h
    src/WipeMethod.h

    # Metrics
    src/metrics/BaseMetric.cpp
    src/metrics/BaseMetric.h
    src/metrics/MetricsManager.cpp
    src/metrics/MetricsManager.h
    src/metrics/MetricsRegistering.cpp
    src/metrics/MetricsRegistering.h
    src/metrics/PerceptualMetric.cpp
    src/metrics/PerceptualMetric.h
    src/metrics/PerChannelMeanMetric.cpp
    src/metrics/PerChannelMeanMetric.h
    src/metrics/PerChannelMetric.cpp
    src/metrics/PerChannelMetric.h
    src/metrics/PerLuminanceMetric.cpp
    src/metrics/PerLuminanceMetric.h

    # UI files
    ui/AboutDialog.ui
    ui/DiffImgWindow.ui
    ui/FilesDialog.ui

    # Resource files
    res/diffimg.qrc

    ## 3rd party
    # Perceptual diff
    3rdparty/perceptualdiff/LPyramid.h
    3rdparty/perceptualdiff/Metric.h
    3rdparty/perceptualdiff/CompareArgs.h
    3rdparty/perceptualdiff/RGBAImage.h
    3rdparty/perceptualdiff/LPyramid.cpp
    3rdparty/perceptualdiff/Metric.cpp
    3rdparty/perceptualdiff/CompareArgs.cpp
    3rdparty/perceptualdiff/RGBAImage.cpp

    # Qt property browser
    3rdparty/qtpropertybrowser/qteditorfactory.cpp
    3rdparty/qtpropertybrowser/qteditorfactory.h
    3rdparty/qtpropertybrowser/qtpropertybrowser.cpp
    3rdparty/qtpropertybrowser/qtpropertybrowser.h
    3rdparty/qtpropertybrowser/qtpropertybrowserutils.cpp
    3rdparty/qtpropertybrowser/qtpropertybrowserutils_p.h
    3rdparty/qtpropertybrowser/qtpropertymanager.cpp
    3rdparty/qtpropertybrowser/qtpropertymanager.h
    3rdparty/qtpropertybrowser/qttreepropertybrowser.cpp
    3rdparty/qtpropertybrowser/qttreepropertybrowser.h
    3rdparty/qtpropertybrowser/qtvariantproperty.cpp
    3rdparty/qtpropertybrowser/qtvariantproperty.h
    )

target_include_directories(diffimg_core PUBLIC
    "${CMAKE_CURRENT_BINARY_DIR}/diffimg_core_autogen/include"
    src
    src/metrics

    3rdparty/perceptualdiff
    3rdparty/qtpropertybrowser
    )
target_link_libraries(diffimg_core PUBLIC Qwt::Qwt Qt6::Widgets)
target_compile_definitions(diffimg_core PUBLIC QWT_DLL NOMINMAX _CRT_SECURE_NO_WARNINGS)
if(MSVC)
    target_compile_options(diffimg_core PUBLIC /utf-8 /Brepro)
    if(DIFFIMG_ASAN)
        target_compile_options(diffimg_core PUBLIC /fsanitize=address /Zi)
        target_link_options(diffimg_core PUBLIC /INCREMENTAL:NO /DEBUG)
    endif()
endif()
add_executable(diffimg WIN32 src/main.cpp)
target_link_libraries(diffimg PRIVATE diffimg_core)
add_executable(diffimg-cli src/main.cpp)
target_compile_definitions(diffimg-cli PRIVATE DIFFIMG_CONSOLE)
target_link_libraries(diffimg-cli PRIVATE diffimg_core)
if(MSVC)
    target_link_options(diffimg PRIVATE /Brepro)
    target_link_options(diffimg-cli PRIVATE /Brepro)
endif()
if(WIN32)
    target_sources(diffimg PRIVATE res/windows.rc)
    target_sources(diffimg-cli PRIVATE res/windows.rc)
endif()
install(TARGETS diffimg diffimg-cli RUNTIME DESTINATION .)
if(BUILD_TESTING)
    find_package(Qt6 COMPONENTS Test REQUIRED)
    add_executable(comparison-tests tests/comparison_tests.cpp)
    target_link_libraries(comparison-tests PRIVATE diffimg_core Qt6::Test)
    add_executable(review-tests tests/review_tests.cpp)
    target_link_libraries(review-tests PRIVATE diffimg_core Qt6::Test)
    add_test(NAME project-review COMMAND review-tests -platform windows -o review-results.xml,xml)
    add_test(NAME comparisons COMMAND comparison-tests -platform windows -o comparison-results.xml,xml)
endif()
