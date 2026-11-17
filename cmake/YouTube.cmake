# Optional embedded YouTube player. Downloads (at configure time) the webview library and the Microsoft WebView2 SDK
# headers from NuGet. If a download fails, the feature is simply left out and the build continues.
set(_hat_ok TRUE)

function(hat_fetch url file dest okvar)
    if(NOT EXISTS "${dest}")
        file(DOWNLOAD "${url}" "${CMAKE_BINARY_DIR}/${file}" STATUS st TIMEOUT 180 TLS_VERIFY ON)
        list(GET st 0 code)
        if(NOT code EQUAL 0)
            message(WARNING "YouTube support: could not download ${url} (${st})")
            set(${okvar} FALSE PARENT_SCOPE)
            return()
        endif()
        file(MAKE_DIRECTORY "${dest}")
        file(ARCHIVE_EXTRACT INPUT "${CMAKE_BINARY_DIR}/${file}" DESTINATION "${dest}")
    endif()
    set(${okvar} TRUE PARENT_SCOPE)
endfunction()

hat_fetch("https://github.com/webview/webview/archive/refs/tags/0.12.0.tar.gz" "webview-0.12.0.tar.gz"
          "${CMAKE_BINARY_DIR}/_deps/webview" _ok_wv)
hat_fetch("https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/1.0.1150.38" "webview2-1.0.1150.38.zip"
          "${CMAKE_BINARY_DIR}/_deps/mswebview2" _ok_w2)

set(_wv_inc "${CMAKE_BINARY_DIR}/_deps/webview/webview-0.12.0/core/include")
set(_wv_compat "${CMAKE_BINARY_DIR}/_deps/webview/webview-0.12.0/compatibility/mingw/include")
set(_w2_inc "${CMAKE_BINARY_DIR}/_deps/mswebview2/build/native/include")

if(_ok_wv AND _ok_w2 AND EXISTS "${_wv_inc}/webview.h" AND EXISTS "${_w2_inc}/WebView2.h")
    target_include_directories(Hatenium PRIVATE "${_wv_inc}" "${_wv_compat}" "${_w2_inc}")
    target_compile_definitions(Hatenium PRIVATE HATENIUM_YOUTUBE=1)
    target_link_libraries(Hatenium PRIVATE ole32 shell32 shlwapi version advapi32 oleaut32)
    message(STATUS "YouTube support: enabled (WebView2 SDK + webview found)")
else()
    message(WARNING "YouTube support: disabled (WebView2 SDK or webview library not available)")
endif()
