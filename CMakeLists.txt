cmake_minimum_required(VERSION 3.10)
project(monopoly C)

add_library(monopoly SHARED src/monopoly.c src/monopoly_init.c src/app.c)

target_include_directories(monopoly PRIVATE 
    ../pilotlight/src 
    ../pilotlight/libs 
    ../pilotlight/extensions 
    ../pilotlight/dependencies/stb)

target_compile_definitions(monopoly PRIVATE
    $<$<CONFIG:Debug>:_DEBUG PL_CONFIG_DEBUG>
    $<$<CONFIG:Release>:NDEBUG PL_CONFIG_RELEASE>)

set_target_properties(monopoly PROPERTIES 
    C_STANDARD 11
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/out)


target_compile_options(monopoly PRIVATE -Zc:preprocessor -nologo 
    -W4 -WX -wd4201 -wd4100 -wd4996 -wd4505 -wd4189 -wd5105 -wd4115 -permissive- 
    $<$<CONFIG:Debug>:-Od -MDd -Zi> 
    $<$<CONFIG:Release>:-O2 -MD>)

target_link_options(monopoly PRIVATE -noimplib -noexp -incremental:no)