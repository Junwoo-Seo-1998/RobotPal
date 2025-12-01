# CMakeList.txt: CMakeProject1에 대한 CMake 프로젝트, 여기에 소스를 포함하고
# 프로젝트 특정 논리를 정의합니다.
#
include(FetchContent)

FetchContent_Declare(
	glm
	GIT_REPOSITORY	https://github.com/g-truc/glm.git
	GIT_TAG 	a532f5b1cf27d6a3c099437e6959cf7e398a0a67 #refs/tags/1.0.2
)

FetchContent_MakeAvailable(glm)


FetchContent_Declare(imgui_external
	GIT_REPOSITORY "https://github.com/ocornut/imgui.git"
	GIT_TAG "v1.92.4-docking"
	GIT_SHALLOW 1
	EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(imgui_external)

add_library(imgui STATIC
	${imgui_external_SOURCE_DIR}/imgui.cpp
	${imgui_external_SOURCE_DIR}/imgui_demo.cpp
	${imgui_external_SOURCE_DIR}/imgui_draw.cpp
	${imgui_external_SOURCE_DIR}/imgui_tables.cpp
	${imgui_external_SOURCE_DIR}/imgui_widgets.cpp
	${imgui_external_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
	${imgui_external_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC ${imgui_external_SOURCE_DIR} ${imgui_external_SOURCE_DIR}/backends/)


FetchContent_Declare(
    tinygltf
    GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
    GIT_TAG        v2.9.7
)

set(TINYGLTF_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(TINYGLTF_BUILD_TESTS OFF CACHE INTERNAL "")
set(TINYGLTF_INSTALL OFF CACHE INTERNAL "")

FetchContent_MakeAvailable(tinygltf)


set(FLECS_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(FLECS_PIC ON CACHE BOOL "" FORCE)          # Position Independent Code
set(FLECS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(FLECS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FLECS_APP OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    flecs
    GIT_REPOSITORY https://github.com/SanderMertens/flecs.git
    GIT_TAG        v4.1.2
)

FetchContent_MakeAvailable(flecs)



add_library(glad STATIC ${CMAKE_SOURCE_DIR}/External/glad/src/gles2.c)
target_include_directories(glad PUBLIC ${CMAKE_SOURCE_DIR}/External/glad/include)


file(GLOB_RECURSE ROBOTPAL_SOURCES 
    CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/RobotPal/src/**.cpp"
)
add_executable(RobotPal ${ROBOTPAL_SOURCES})

target_compile_definitions(RobotPal PRIVATE GLFW_INCLUDE_NONE)

target_include_directories(RobotPal PUBLIC
   ${CMAKE_SOURCE_DIR}/RobotPal/include
)

target_link_libraries(RobotPal PRIVATE glm::glm)
target_link_libraries(RobotPal PRIVATE glad)
target_link_libraries(RobotPal PRIVATE tinygltf)
target_link_libraries(RobotPal PRIVATE flecs::flecs_static)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET RobotPal PROPERTY CXX_STANDARD 17)
endif()

set(ASSET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Assets")
message(Asset Dir: ${ASSET_DIR})

# 플랫폼에 따라 링크 라이브러리를 다르게 설정합니다.
if(EMSCRIPTEN)
    add_custom_command(TARGET RobotPal
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_SOURCE_DIR}/RobotPal-web/template.html"
            "${CMAKE_CURRENT_BINARY_DIR}/index.html"
    )

    # Emscripten의 경우 플랫폼별 GL 라이브러리 없이 링크합니다.
    target_link_libraries(imgui PRIVATE glfw)
    target_link_libraries(RobotPal PRIVATE glfw imgui)

    # Emscripten 관련 속성 및 링크 플래그를 설정합니다.
    set_target_properties(RobotPal PROPERTIES SUFFIX ".html")
    target_link_options(RobotPal PRIVATE
        "-sUSE_GLFW=3"
        "-sFULL_ES3=1"          # WebGL 2에 OpenGL ES 3 사용
        "-sALLOW_MEMORY_GROWTH=1"
        "--use-preload-plugins" # 에셋 로딩을 위함
        "-sPROXY_TO_PTHREAD"
        "-sPTHREAD_POOL_SIZE=3"
    )
    target_link_options(RobotPal PRIVATE
        "SHELL:--preload-file ${ASSET_DIR}@/Assets"
    )

    target_link_options(RobotPal PRIVATE
        "SHELL:-sSTACK_SIZE=8388608"
    )
else()
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(glfw)

    find_package(OpenGL REQUIRED)

    # 3. 라이브러리 링크
    # "opengl32" 대신 "OpenGL::GL" 사용
    # (참고: 네이티브 빌드 시 glfw 타겟은 Linux에서 X11 등을 자동으로 찾아 링크합니다)
    target_link_libraries(imgui PRIVATE glfw OpenGL::GL)
    target_link_libraries(RobotPal PRIVATE glfw OpenGL::GL imgui)

    add_custom_command(TARGET RobotPal POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${ASSET_DIR}
        $<TARGET_FILE_DIR:RobotPal>/Assets
        COMMENT "Copying Assets to output directory..."
    )

    # 6. Linux 전용 X11 라이브러리 처리
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        find_package(X11 REQUIRED)
        # imgui_impl_glfw.cpp가 X11 함수를 직접 사용하므로 imgui에 링크
        target_link_libraries(imgui PRIVATE ${X11_LIBRARIES})
    endif()
endif()

# TODO: 필요한 경우 테스트를 추가하고 대상을 설치합니다.
