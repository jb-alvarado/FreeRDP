# Exercise the real client code with SDL's headless software renderer.
set(SDL_TEST_SRCS)
foreach(SOURCE IN LISTS SRCS)
  if(NOT SOURCE STREQUAL "sdl_freerdp.cpp")
    if(IS_ABSOLUTE "${SOURCE}")
      list(APPEND SDL_TEST_SRCS "${SOURCE}")
    else()
      list(APPEND SDL_TEST_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/../${SOURCE}")
    endif()
  endif()
endforeach()

# Keep the client entry point out of test executables while retaining its helpers.
add_library(sdl3-test-client STATIC ${SDL_TEST_SRCS} SDLTestClient.cpp)
target_include_directories(
  sdl3-test-client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_link_libraries(sdl3-test-client PUBLIC ${LIBS})
if(SDL_X11_NATIVE)
  target_compile_definitions(sdl3-test-client PRIVATE WITH_SDL_X11_NATIVE)
  target_include_directories(sdl3-test-client PRIVATE ${X11_INCLUDE_DIR})
endif()
if(SDL_WAYLAND_NATIVE)
  target_compile_definitions(sdl3-test-client PRIVATE WITH_SDL_WAYLAND_NATIVE)
  target_include_directories(sdl3-test-client PRIVATE ${WAYLAND_CLIENT_INCLUDE_DIRS})
endif()

function(add_sdl_regression_test NAME)
  add_executable(${NAME} ${NAME}.cpp ${ARGN})
  target_link_libraries(${NAME} PRIVATE sdl3-test-client)
  add_test(NAME ${NAME} COMMAND ${NAME})
  set_tests_properties(
    ${NAME} PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_RENDER_DRIVER=software" TIMEOUT 10
  )
endfunction()

find_package(SDL3_ttf REQUIRED)
add_sdl_regression_test(TestSDLDialogRepaint SDLTestDialogRepaint.cpp)
if(WITH_SDL_LINK_SHARED)
  target_link_libraries(TestSDLDialogRepaint PRIVATE SDL3_ttf::SDL3_ttf)
else()
  target_link_libraries(TestSDLDialogRepaint PRIVATE SDL3_ttf::SDL3_ttf-static)
endif()

add_sdl_regression_test(TestSDLDialogScale SDLTestDialogScale.cpp)

add_sdl_regression_test(TestSDLRepaint)

add_sdl_regression_test(TestSDLClippedRepaint)

add_sdl_regression_test(TestSDLDisplayHandover)

add_sdl_regression_test(TestSDLMainLoop)
