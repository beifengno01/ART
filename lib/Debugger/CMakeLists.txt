SET(TARGET_NAME Debugger)

FILE(GLOB_RECURSE GUI_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*.c* ${CMAKE_CURRENT_SOURCE_DIR}/*.h* ${ART_INCLUDE_DIR}/${TARGET_NAME}/*.h)
aux_source_directory(Jdwp JDWPSRC)


INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

set_source_files_properties(
        Jdwp/JdwpReader.cpp Jdwp/Request.cpp
        PROPERTIES
        COMPILE_FLAGS -Wno-cast-align
)

add_library(${TARGET_NAME} STATIC ${GUI_SRCS} ${JDWPSRC})
target_link_libraries(${TARGET_NAME} utils SmaliAnalysis Qt5::WebSockets Qt5::Widgets)
