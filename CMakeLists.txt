CMAKE_MINIMUM_REQUIRED(VERSION 3.21)
PROJECT(ClienServerEcn)

include(FetchContent)
FIND_PACKAGE(Boost 1.40 COMPONENTS system filesystem unit_test_framework REQUIRED)
FIND_PACKAGE(Threads REQUIRED)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${UNIT_TESTS_INCLUDES})

SET(CMAKE_CXX_STANDARD 20)

FetchContent_Declare(
  libpqxx
  GIT_REPOSITORY https://github.com/jtv/libpqxx.git
  GIT_TAG a6b1d60e74c1427c8ac2324b85cd4a0dc2068332
)
set(PQXX_LIBRARIES pqxx_static)

FetchContent_MakeAvailable(libpqxx)


ADD_LIBRARY(CommonLibrary OBJECT common/json.hpp common/Common.hpp)
target_include_directories(
  CommonLibrary PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/common>"
)
SET_TARGET_PROPERTIES(CommonLibrary PROPERTIES LINKER_LANGUAGE CXX)

ADD_LIBRARY(ClientLibrary OBJECT client/Client.hpp client/Client.cpp)
target_include_directories(
  ClientLibrary PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/client>"
)
target_link_libraries(ClientLibrary PUBLIC CommonLibrary "$<TARGET_OBJECTS:CommonLibrary>")

ADD_LIBRARY(ServerLibrary OBJECT server/Server.hpp server/Server.cpp)
target_include_directories(
  ServerLibrary PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server>"
)
target_link_libraries(ServerLibrary PUBLIC CommonLibrary "$<TARGET_OBJECTS:CommonLibrary>" "${PQXX_LIBRARIES}")

ADD_EXECUTABLE(Server main_server.cpp)
TARGET_LINK_LIBRARIES(Server PRIVATE ServerLibrary Threads::Threads ${Boost_LIBRARIES})

ADD_EXECUTABLE(Client main_client.cpp)
TARGET_LINK_LIBRARIES(Client PRIVATE ClientLibrary Threads::Threads ${Boost_LIBRARIES})

ADD_EXECUTABLE(Test test/test.cpp)
TARGET_LINK_LIBRARIES(Test PRIVATE ClientLibrary ServerLibrary Threads::Threads 
	${Boost_LIBRARIES} 
	${Boost_FILESYSTEM_LIBRARY} 
	${Boost_SYSTEM_LIBRARY} 
	${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

TARGET_INCLUDE_DIRECTORIES(Test PUBLIC 
				"${PROJECT_BINARY_DIR}"
                        "${PROJECT_SOURCE_DIR}/common"
				"${PROJECT_SOURCE_DIR}/client"
				"${PROJECT_SOURCE_DIR}/server")
