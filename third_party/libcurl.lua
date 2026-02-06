group("third_party")
project("libcurl")
  uuid("1ba7e608-5752-457c-8df0-c006c6e8b7fe")
  kind("StaticLib")
  language("C")

  defines({
    "BUILDING_LIBCURL",
    "CURL_STATICLIB",
    "HTTP_ONLY",  -- We only need HTTP
    "CURL_DISABLE_LDAP",
    "CURL_DISABLE_LDAPS",
    "CURL_DISABLE_DICT",
    "CURL_DISABLE_FILE",
    "CURL_DISABLE_FTP",
    "CURL_DISABLE_GOPHER",
    "CURL_DISABLE_IMAP",
    "CURL_DISABLE_POP3",
    "CURL_DISABLE_RTSP",
    "CURL_DISABLE_SMB",
    "CURL_DISABLE_SMTP",
    "CURL_DISABLE_TELNET",
    "CURL_DISABLE_TFTP",
  })

  filter("platforms:Windows")
    defines({
      "USE_SCHANNEL",
      "USE_WINDOWS_SSPI",
    })
    links({
      "Wldap32",
      "crypt32",
      "ws2_32",
    })

  filter("platforms:Mac*")
    defines({
      "USE_OPENSSL",
      "USE_APPLE_SECTRUST",
      "HAVE_CONFIG_H",
      "OPENSSL_SUPPRESS_DEPRECATED",
    })
    sysincludedirs({
      "/usr/local/opt/openssl@3/include",
      "/opt/homebrew/opt/openssl@3/include",
    })
    libdirs({
      "/usr/local/opt/openssl@3/lib",
      "/opt/homebrew/opt/openssl@3/lib",
    })
    links({
      "ssl",
      "crypto",
      "SystemConfiguration.framework",
      "CoreFoundation.framework",
    })
    buildoptions({
      "-Wno-deprecated-declarations",
    })

  filter("platforms:Linux")
    defines({
      "USE_OPENSSL",
      "HAVE_CONFIG_H",
    })
    links({
      "ssl",
      "crypto",
    })

  filter("configurations:Release")
    buildoptions({
      "-Os",
    })

  filter({})

  includedirs({
    "libcurl/lib",
  })

  -- Use sysincludedirs for the public curl headers so they're found by
  -- #include <curl/...> (angled bracket includes)
  sysincludedirs({
    "libcurl/include",
  })

  files({
    "libcurl/lib/**.h",
    "libcurl/lib/**.c",
  })

  -- Exclude Windows-specific files on non-Windows
  filter("platforms:not Windows")
    excludes({
      "libcurl/lib/**/schannel*",
      "libcurl/lib/**/system_win32*",
    })

  filter({})
