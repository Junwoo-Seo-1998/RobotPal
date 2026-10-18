cmake_minimum_required(VERSION 3.8)
project(RobotPal)

# ==============================================================================
# [핵심 수정] Emscripten 전역 설정 (반드시 맨 위에 위치)
# 외부 라이브러리(Flecs 등)도 -pthread 옵션을 먹도록 add_compile_options 사용
# ==============================================================================
if(EMSCRIPTEN)
    # 1. 환경 변수 자동 설정
    if("$ENV{EMSCRIPTEN}" STREQUAL "")
        message(STATUS "EMSCRIPTEN environment variable not found. Auto-detecting...")
        get_filename_component(EMSCRIPTEN_ROOT_PATH ${CMAKE_C_COMPILER} DIRECTORY)
        set(ENV{EMSCRIPTEN} "${EMSCRIPTEN_ROOT_PATH}")
    endif()

    # 2. 컴파일 옵션 (모든 라이브러리에 적용됨 - 중요!)
    # 여기서 -pthread를 줘야 Flecs 등이 atomics 기능을 켜고 컴파일됩니다.
    add_compile_options("-pthread") 

    # 3. 링크 옵션 (전역 설정)
    # 스레드 풀 사이즈, 웹소켓 등 링커에게 필요한 정보를 미리 줍니다.
    add_link_options(
        "-pthread"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sPTHREAD_POOL_SIZE=8"   # 스레드 풀
        "-sUSE_GLFW=3"
        "-sFULL_ES3=1"
        "-lwebsocket.js"
        "--use-preload-plugins"
        "SHELL:-sSTACK_SIZE=8388608"
        "-sEXPORTED_RUNTIME_METHODS=[ccall,HEAPU8]"
        "-sEXPORTED_FUNCTIONS=[_main,_malloc,_free]"
    )

endif()

# ==============================================================================
# 라이브러리 로드 (FetchContent)
# ==============================================================================
include(FetchContent)

set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    glm
    GIT_REPOSITORY  https://github.com/g-truc/glm.git
    GIT_TAG         a532f5b1cf27d6a3c099437e6959cf7e398a0a67 
    GIT_SHALLOW 1
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

FetchContent_Declare(imgui_file_dialog_external
    GIT_REPOSITORY "https://github.com/aiekick/ImGuiFileDialog.git"
    GIT_TAG "v0.6.8"
    GIT_SHALLOW 1
    EXCLUDE_FROM_ALL
)
FetchContent_Populate(imgui_file_dialog_external)

add_library(imgui_file_dialog STATIC
    ${imgui_file_dialog_external_SOURCE_DIR}/ImGuiFileDialog.cpp
)
target_include_directories(imgui_file_dialog PUBLIC ${imgui_file_dialog_external_SOURCE_DIR})
target_link_libraries(imgui_file_dialog PUBLIC imgui)


FetchContent_Declare(imgui_guizmo_external
    GIT_REPOSITORY "https://github.com/CedricGuillemet/ImGuizmo.git"
    GIT_SHALLOW 1
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(imgui_guizmo_external)

add_library(imgui_guizmo STATIC
    ${imgui_guizmo_external_SOURCE_DIR}/src/GraphEditor.cpp
    ${imgui_guizmo_external_SOURCE_DIR}/src/GraphEditor.h
    ${imgui_guizmo_external_SOURCE_DIR}/src/ImCurveEdit.cpp
    ${imgui_guizmo_external_SOURCE_DIR}/src/ImCurveEdit.h
    ${imgui_guizmo_external_SOURCE_DIR}/src/ImGradient.cpp
    ${imgui_guizmo_external_SOURCE_DIR}/src/ImGradient.h
    ${imgui_guizmo_external_SOURCE_DIR}/src/ImGuizmo.cpp
    ${imgui_guizmo_external_SOURCE_DIR}/src/ImGuizmo.h
    ${imgui_guizmo_external_SOURCE_DIR}/src/ImSequencer.cpp
    ${imgui_guizmo_external_SOURCE_DIR}/src/ImSequencer.h
    ${imgui_guizmo_external_SOURCE_DIR}/src/ImZoomSlider.h
)

target_include_directories(imgui_guizmo PUBLIC ${imgui_guizmo_external_SOURCE_DIR}/src)
target_link_libraries(imgui_guizmo PUBLIC imgui)

# Draco 및 기타 라이브러리
set(DRACO_TESTS OFF CACHE BOOL "" FORCE)
set(DRACO_JS_GLUE OFF CACHE BOOL "" FORCE)
set(DRACO_WASM OFF CACHE BOOL "" FORCE)
set(DRACO_TRANSCODER_SUPPORTED OFF CACHE BOOL "" FORCE)
set(DRACO_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    draco
    GIT_REPOSITORY https://github.com/google/draco.git
    GIT_TAG        1.5.7
    GIT_SHALLOW 1
)
FetchContent_MakeAvailable(draco)

FetchContent_Declare(
    tinygltf
    GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
    GIT_TAG        v2.9.7
    GIT_SHALLOW 1
)
set(TINYGLTF_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(TINYGLTF_BUILD_TESTS OFF CACHE INTERNAL "")
set(TINYGLTF_INSTALL OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(tinygltf)

set(FLECS_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(FLECS_PIC ON CACHE BOOL "" FORCE)
set(FLECS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(FLECS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FLECS_APP OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    flecs
    GIT_REPOSITORY https://github.com/SanderMertens/flecs.git
    GIT_TAG        v4.1.2
    GIT_SHALLOW 1
)
FetchContent_MakeAvailable(flecs)

FetchContent_Declare(
    stb_external
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
    GIT_SHALLOW 1
)
FetchContent_MakeAvailable(stb_external)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_external_SOURCE_DIR})

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)

add_library(glad STATIC ${CMAKE_SOURCE_DIR}/External/glad/src/gles2.c)
target_include_directories(glad PUBLIC ${CMAKE_SOURCE_DIR}/External/glad/include)


# ==============================================================================
# 타겟 생성 (add_executable)
# ==============================================================================
file(GLOB_RECURSE ROBOTPAL_SOURCES 
    CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/RobotPal/src/**.cpp"
)
add_executable(RobotPal ${ROBOTPAL_SOURCES})

target_compile_definitions(RobotPal PRIVATE GLFW_INCLUDE_NONE)
target_include_directories(RobotPal PUBLIC ${CMAKE_SOURCE_DIR}/RobotPal/include)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET RobotPal PROPERTY CXX_STANDARD 17)
endif()

set(ASSET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Assets")
message(Asset Dir: ${ASSET_DIR})

# 공통 라이브러리 링크
target_link_libraries(RobotPal PRIVATE glm::glm)
target_link_libraries(RobotPal PRIVATE glad)
target_link_libraries(RobotPal PUBLIC draco::draco)
target_include_directories(RobotPal PUBLIC "${CMAKE_BINARY_DIR}" ${draco_SOURCE_DIR}/src)
target_link_libraries(RobotPal PRIVATE tinygltf)
target_link_libraries(RobotPal PRIVATE flecs::flecs_static)
target_link_libraries(RobotPal PRIVATE stb)
target_link_libraries(RobotPal PRIVATE nlohmann_json::nlohmann_json)


# ==============================================================================
# 플랫폼별 추가 설정 (Target Specific)
# ==============================================================================
if(EMSCRIPTEN)
    # Emscripten 라이브러리 링크
    target_link_libraries(imgui PRIVATE glfw)
    target_link_libraries(RobotPal PRIVATE glfw imgui imgui_file_dialog imgui_guizmo)

    # 에셋 프리로딩 설정 (ASSET_DIR 변수가 필요하므로 여기서 설정)
    target_link_options(RobotPal PRIVATE
        "SHELL:--preload-file ${ASSET_DIR}@/Assets"
    )

    set_target_properties(RobotPal PROPERTIES SUFFIX ".html")

    add_custom_command(TARGET RobotPal
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_SOURCE_DIR}/RobotPal-web/template.html"
            "${CMAKE_CURRENT_BINARY_DIR}/index.html"

    COMMAND ${CMAKE_COMMAND} -E copy
           "${CMAKE_SOURCE_DIR}/RobotPal-web/coi-serviceworker.min.js"
           "${CMAKE_CURRENT_BINARY_DIR}/coi-serviceworker.min.js"
    )

else()
    # Native (Windows/Linux) 빌드 설정
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
    find_package(Threads REQUIRED)

    target_link_libraries(RobotPal PRIVATE Threads::Threads)
    target_link_libraries(imgui PRIVATE glfw OpenGL::GL)
    target_link_libraries(RobotPal PRIVATE glfw OpenGL::GL imgui imgui_file_dialog imgui_guizmo)

    add_custom_command(TARGET RobotPal POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${ASSET_DIR}
        $<TARGET_FILE_DIR:RobotPal>/Assets
        COMMENT "Copying Assets to output directory..."
    )

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        find_package(X11 REQUIRED)
        target_link_libraries(imgui PRIVATE ${X11_LIBRARIES})
    endif()
endif()

# ==============================================================================
# [최종 수정] libjpeg-turbo 설정 (Native vs Web 분기)
# ==============================================================================

if(EMSCRIPTEN)
    # --------------------------------------------------------------------------
    # 1. Web (Emscripten) 환경
    # --------------------------------------------------------------------------
    message(STATUS "Configuring libjpeg for Emscripten (Using built-in Port)")
    
    # Emscripten은 libjpeg를 자체적으로 내장하고 있습니다 ('-s USE_LIBJPEG=1').
    # 굳이 소스를 받아서 빌드할 필요가 없습니다.
    
    # 컴파일/링크 옵션에 포트 사용 설정 추가
    target_compile_options(RobotPal PRIVATE "-sUSE_LIBJPEG=1")
    target_link_options(RobotPal PRIVATE "-sUSE_LIBJPEG=1")
    
    # (참고) 별도의 target_link_libraries가 필요 없습니다. 
    # 하지만 코드 호환성을 위해 가짜 타겟이 필요하다면 아래처럼 처리 가능합니다.

    else()
    # --------------------------------------------------------------------------
    # 2. Native (Windows/Linux) 환경
    # --------------------------------------------------------------------------
    message(STATUS "Configuring libjpeg for Native (Using ExternalProject + BYPRODUCTS)")

    # 1. Native 전용 라이브러리들 (GLFW 등)
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
    find_package(Threads REQUIRED)

    target_link_libraries(RobotPal PRIVATE Threads::Threads)
    target_link_libraries(imgui PRIVATE glfw OpenGL::GL)
    target_link_libraries(RobotPal PRIVATE glfw OpenGL::GL imgui imgui_file_dialog imgui_guizmo)

    add_custom_command(TARGET RobotPal POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${ASSET_DIR}
        $<TARGET_FILE_DIR:RobotPal>/Assets
        COMMENT "Copying Assets to output directory..."
    )

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        find_package(X11 REQUIRED)
        target_link_libraries(imgui PRIVATE ${X11_LIBRARIES})
    endif()

    # --------------------------------------------------------------------------
    # [수정] libjpeg-turbo (ExternalProject)
    # --------------------------------------------------------------------------
    include(ExternalProject)
    
    set(LIBJPEG_INSTALL_DIR "${CMAKE_BINARY_DIR}/libjpeg_turbo_install")
    
    # [중요] ExternalProject_Add 호출 전에 파일명을 먼저 결정해야 합니다.
    if(MSVC)
        set(LIBJPEG_LIB_NAME "jpeg-static.lib")
    else()
        set(LIBJPEG_LIB_NAME "libjpeg.a")
    endif()

    ExternalProject_Add(
        libjpeg_turbo
        GIT_REPOSITORY https://github.com/libjpeg-turbo/libjpeg-turbo.git
        GIT_TAG 3.0.3
        PREFIX ${CMAKE_BINARY_DIR}/libjpeg_turbo_build
        # [핵심 수정] Ninja에게 이 파일이 생성될 것임을 약속합니다.
        BUILD_BYPRODUCTS "${LIBJPEG_INSTALL_DIR}/lib/${LIBJPEG_LIB_NAME}"
        CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX=${LIBJPEG_INSTALL_DIR}
            -DENABLE_STATIC=ON
            -DENABLE_SHARED=OFF
            -DWITH_SIMD=OFF  
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    )

    # 폴더 미리 생성 (Configure 에러 방지)
    file(MAKE_DIRECTORY "${LIBJPEG_INSTALL_DIR}/include")
    
    add_library(libjpeg_turbo_static STATIC IMPORTED)
    set_target_properties(libjpeg_turbo_static PROPERTIES
        IMPORTED_LOCATION "${LIBJPEG_INSTALL_DIR}/lib/${LIBJPEG_LIB_NAME}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBJPEG_INSTALL_DIR}/include"
    )
    
    add_dependencies(libjpeg_turbo_static libjpeg_turbo)
    target_link_libraries(RobotPal PRIVATE libjpeg_turbo_static)

endif()
