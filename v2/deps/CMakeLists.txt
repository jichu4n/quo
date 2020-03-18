include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.10.0
)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
  abseil
  GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
  GIT_TAG 20200225.1
)
FetchContent_MakeAvailable(abseil)

set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.7.3
)
FetchContent_MakeAvailable(json)

set(INJA_USE_EMBEDDED_JSON OFF CACHE BOOL "" FORCE)
set(INJA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_BENCHMARK OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  inja
  GIT_REPOSITORY https://github.com/pantor/inja.git
  GIT_TAG cb0b852
)
FetchContent_MakeAvailable(inja)

