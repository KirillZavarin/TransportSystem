# CMakeList.txt: проект CMake для TransportSystem; включите исходный код и определения,
# укажите здесь логику для конкретного проекта.
#
cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)

find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS transport_catalogue.proto map_renderer.proto svg.proto transport_router.proto)

set(FILES_TO_WORK_WITH_JSON json.h json.cpp json_builder.h json_builder.cpp json_reader.h json_reader.cpp)

set(FILES_TO_WORK_WITH_MAP svg.h svg.cpp map_renderer.h map_renderer.cpp)

set(FILES_TO_WORK_WITH_ROUTE router.h ranges.h graph.h transport_router.h transport_router.cpp)

set(TRANSPORT_CATALOGUE_FILES geo.h geo.cpp domain.h domain.cpp request_handler.h request_handler.cpp transport_catalogue.h transport_catalogue.cpp)

# Добавьте источник в исполняемый файл этого проекта.
add_executable("transport_catalogue" ${PROTO_SRCS} ${PROTO_HDRS} ${FILES_TO_WORK_WITH_JSON} ${FILES_TO_WORK_WITH_MAP} ${FILES_TO_WORK_WITH_ROUTE} ${TRANSPORT_CATALOGUE_FILES} serialization.h serialization.cpp main.cpp)

target_include_directories(transport_catalogue PUBLIC ${Protobuf_INCLUDE_DIRS})
target_include_directories(transport_catalogue PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

string(REPLACE "protobuf.lib" "protobufd.lib" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")
string(REPLACE "protobuf.a" "protobufd.a" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")

target_link_libraries(transport_catalogue "$<IF:$<CONFIG:Debug>,${Protobuf_LIBRARY_DEBUG},${Protobuf_LIBRARY}>" Threads::Threads)

# TODO: Добавьте тесты и целевые объекты, если это необходимо.
