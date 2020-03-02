cmake_minimum_required(VERSION 3.1)

project(ffmpeg C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CompileOptions -Wall -Werror -g)
add_compile_options(${CompileOptions})

include_directories(
	"D:/3rdparty/release/include"
)

link_directories(
	"D:/3rdparty/release/lib"
)

add_executable(main main.cpp MediaHandle.cpp MediaBaseFunc.cpp)

target_link_libraries(main PUBLIC avfilter avformat avdevice avcodec swscale avutil swresample Secur32 bcrypt z m ws2_32 iconv)