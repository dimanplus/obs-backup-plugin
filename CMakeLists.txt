cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

# ОБЯЗАТЕЛЬНО: включаем использование frontend API и Qt
set(ENABLE_FRONTEND_API ON)
set(ENABLE_QT ON)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

# OBS core
find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

# OBS frontend API (док-виджеты и main_window)
find_package(obs-frontend-api REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME}
  PRIVATE
    OBS::obs-frontend-api
)

# Qt
find_package(Qt6 REQUIRED COMPONENTS Widgets Core)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)
target_compile_options(${CMAKE_PROJECT_NAME}
  PRIVATE
    $<$<C_COMPILER_ID:MSVC>:/wd4996>
    $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-deprecated-declarations>
)

set_target_properties(
  ${CMAKE_PROJECT_NAME}
  PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
)

# Source files
target_sources(${CMAKE_PROJECT_NAME}
  PRIVATE
    src/plugin-main.cpp
    src/test.cpp
)

# Set plugin output name
set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})
