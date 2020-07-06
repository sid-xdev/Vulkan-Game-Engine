

add_library( androidlib OBJECT AndroidSurface.hpp AndroidSurface.cpp )

target_include_directories( androidlib PUBLIC ${ANDROID_NDK}/sources/android/native_app_glue )

target_compile_features( androidlib PUBLIC cxx_std_17 )

add_definitions( -DVK_USE_PLATFORM_ANDROID_KHR )
add_definitions( -DVULKAN_HPP_NO_EXCEPTIONS )