cmake_minimum_required(VERSION 3.15) 
# 指定使用的最低 CMake 版本。确保语法和功能兼容。

project(demo_project LANGUAGES CXX) 
# 定义项目名称为 demo_project，指定项目主要使用 C++ 语言。

set(CMAKE_CXX_STANDARD 20) 
# 设置 C++ 标准为 C++20。

set(CMAKE_CXX_STANDARD_REQUIRED ON) 
# 表示必须使用指定的 C++ 标准，不允许降级。

# 根据构建类型添加编译选项
if (CMAKE_BUILD_TYPE MATCHES Debug) 
    add_compile_options(-Wall -g) 
    # Debug 模式下开启所有警告(-Wall)并生成调试信息(-g)
else()
    add_compile_options(-Wall -O2) 
    # Release 模式下开启所有警告，并优化级别为 O2
endif()

# ======== 添加 spdlog ========
find_package(spdlog REQUIRED)  # ⭐️ 查找 spdlog（要求你用过 vcpkg/conan/apt 等安装过）



# 设置 XTP API 的头文件和库文件目录（根据实际路径修改）
set(PROJECT_ROOT "/root/my_xtp") 
set(XTP_INCLUDE_DIR "${PROJECT_ROOT}/api") 
set(XTP_INCLUDE_DIR2 "${PROJECT_ROOT}/include") 
set(XTP_LIB_DIR "${PROJECT_ROOT}/lib") 

# 添加头文件搜索路径
include_directories( 
    ${XTP_INCLUDE_DIR} 
    ${XTP_INCLUDE_DIR2} 
)

# 自动查找当前目录下所有 cpp 文件
file(GLOB SOURCES "*.cpp") 

# 添加可执行文件，名字为 demo，源文件为 SOURCES
add_executable(demo ${SOURCES}) 

# 添加链接库所在目录
target_link_directories(demo PRIVATE ${XTP_LIB_DIR}) 

# 链接库文件
target_link_libraries(demo 
    xtpquoteapi 
    xtptraderapi 
	spdlog::spdlog      # ⭐️ 链接 spdlog（使用官方提供的 CMake 接口）
)

# 设置运行时动态库搜索路径，保证程序运行时能找到动态库
set_target_properties(demo PROPERTIES 
    BUILD_WITH_INSTALL_RPATH TRUE 
    INSTALL_RPATH "${XTP_LIB_DIR}" 
)

# 设置可执行文件的输出目录
set_target_properties(demo PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_ROOT}/src/build" 
)

# 自定义清理目标，用于删除生成的可执行文件和对象文件
add_custom_target(clean_all
    COMMAND ${CMAKE_COMMAND} -E rm -f ${PROJECT_ROOT}/src/demo
    COMMAND ${CMAKE_COMMAND} -E rm -f ${PROJECT_ROOT}/*.o
    COMMENT "Cleaning all built files."
)

