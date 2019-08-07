cmake_minimum_required(VERSION 3.14)
project(ell
    VERSION 0.21
    DESCRIPTION "Embedded Linux Library"
    HOMEPAGE_URL "https://01.org/ell"
)
set(CMAKE_MODULE_PATH cmake)

option(BUILD_SHARED_LIBS "build ell as a shared library" ON)

option(ENABLE_ASAN "enable linking with address sanitizer" OFF)
if(ENABLE_ASAN)
    find_package(ASan REQUIRED)
    set(SANITIZE_ADDRESS ON)
endif()

option(ENABLE_LSAN "enable linking with leak sanitizer" OFF)
if(ENABLE_LSAN)
    find_package(LSan REQUIRED)
    set(SANITIZE_LEAK ON)
endif()

option(ENABLE_UBSAN "enable linking with undefined behaviour sanitizer" OFF)
if(ENABLE_UBSAN)
    find_package(UBSan REQUIRED)
    set(SANITIZE_UNDEFINED ON)
endif()

include(CheckSymbolExists)
check_symbol_exists(explicit_bzero "string.h" HAVE_EXPLICIT_BZERO)

check_symbol_exists(signalfd "sys/signalfd.h" HAVE_SIGNALFD)
if(NOT HAVE_SIGNALFD)
    message(FATAL_ERROR "signalfd support is required")
endif()

check_symbol_exists(timerfd_create "sys/timerfd.h" HAVE_TIMERFD_CREATE)
if(NOT HAVE_SIGNALFD)
    message(FATAL_ERROR "timerfd support is required")
endif()

check_symbol_exists(epoll_create "sys/epoll.h" HAVE_EPOLL_CREATE)
if(NOT HAVE_SIGNALFD)
    message(FATAL_ERROR "epoll support is required")
endif()

include(CheckLibraryExists)
check_library_exists(dl dlopen "" HAVE_DLOPEN)
if(NOT HAVE_SIGNALFD)
    message(FATAL_ERROR "dynamic linking loader is required")
endif()

include(CheckIncludeFiles)
check_include_files("linux/types.h;linux/if_alg.h" HAVE_LINUX_TYPES_AND_IF_ALG_H)

option(ENABLE_GLIB "enable ell/glib main loop example" OFF)
if(ENABLE_GLIB)
    find_package(Glib 2.32 REQUIRED)
endif()

include(TestBigEndian)
test_big_endian(BIG_ENDIAN)
if(NOT BIG_ENDIAN)
    set(ENABLE_DBUS_TESTS ON)
endif()

find_program(OPENSSL_EXECUTABLE NAMES openssl)
if(OPENSSL_EXECUTABLE)
    set(ENABLE_CERT_TESTS ON)
endif()

include(GNUInstallDirs)
set(prefix "${CMAKE_INSTALL_PREFIX}")
set(exec_prefix "\${prefix}")
set(libdir "\${exec_prefix}/${CMAKE_INSTALL_LIBDIR}")
set(includedir "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
set(VERSION "${CMAKE_PROJECT_VERSION}")
configure_file(ell/ell.pc.in ell.pc @ONLY)

add_library(ell
    ell/util.c
    ell/test.c
    ell/strv.c
    ell/utf8.c
    ell/queue.c
    ell/hashmap.c
    ell/string.c
    ell/settings.c
    ell/main.c
    ell/idle.c
    ell/signal.c
    ell/timeout.c
    ell/io.c
    ell/ringbuf.c
    ell/log.c
    ell/plugin.c
    ell/checksum.c
    ell/netlink.c
    ell/genl.c
    ell/dbus.c
    ell/dbus-message.c
    ell/dbus-util.c
    ell/dbus-service.c
    ell/dbus-client.c
    ell/dbus-name-cache.c
    ell/dbus-filter.c
    ell/gvariant-util.c
    ell/siphash.c
    ell/hwdb.c
    ell/cipher.c
    ell/random.c
    ell/uintset.c
    ell/base64.c
    ell/pem.c
    ell/tls.c
    ell/tls-record.c
    ell/tls-extensions.c
    ell/tls-suites.c
    ell/uuid.c
    ell/key.c
    ell/pkcs5.c
    ell/file.c
    ell/dir.c
    ell/net.c
    ell/dhcp.c
    ell/dhcp-transport.c
    ell/dhcp-lease.c
    ell/cert.c
    ell/ecc-external.c
    ell/ecc.c
    ell/ecdh.c
    ell/time.c
    ell/gpio.c
)

file(READ "Makefile.am" MAKEFILE_AM)
string(REGEX MATCH "ELL_CURRENT = [0-9]+" ELL_CURRENT ${MAKEFILE_AM})
string(REGEX REPLACE "ELL_CURRENT = " "" ELL_CURRENT ${ELL_CURRENT})
string(REGEX MATCH "ELL_REVISION = [0-9]+" ELL_REVISION ${MAKEFILE_AM})
string(REGEX REPLACE "ELL_REVISION = " "" ELL_REVISION ${ELL_REVISION})
string(REGEX MATCH "ELL_AGE = [0-9]+" ELL_AGE ${MAKEFILE_AM})
string(REGEX REPLACE "ELL_AGE = " "" ELL_AGE ${ELL_AGE})
math(EXPR ELL_MAJOR "${ELL_CURRENT} - ${ELL_AGE}" OUTPUT_FORMAT DECIMAL)

set_target_properties(ell PROPERTIES
    VERSION "${ELL_MAJOR}.${ELL_AGE}.${ELL_REVISION}"
    SOVERSION "${ELL_MAJOR}"
    C_VISIBILITY_PRESET "hidden"
)

target_compile_definitions(ell PRIVATE
    UNITDIR="${CMAKE_CURRENT_SOURCE_DIR}/unit/"
    CERTDIR="${CMAKE_CURRENT_SOURCE_DIR}/unit/"
)
if(HAVE_EXPLICIT_BZERO)
    target_compile_definitions(ell PRIVATE HAVE_EXPLICIT_BZERO)
endif()

target_link_options(ell PRIVATE "-Wl,--no-undefined,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/ell/ell.sym")
target_link_libraries(ell PUBLIC "${CMAKE_DL_LIBS}")

install(TARGETS ell)
install(FILES
    ell/base64.h
    ell/cert.h
    ell/checksum.h
    ell/cipher.h
    ell/dbus-client.h
    ell/dbus-service.h
    ell/dbus.h
    ell/dhcp.h
    ell/dir.h
    ell/ecc.h
    ell/ecdh.h
    ell/ell.h
    ell/file.h
    ell/genl.h
    ell/gpio.h
    ell/hashmap.h
    ell/hwdb.h
    ell/idle.h
    ell/io.h
    ell/key.h
    ell/log.h
    ell/main.h
    ell/missing.h
    ell/net.h
    ell/netlink.h
    ell/pem.h
    ell/pkcs5.h
    ell/plugin.h
    ell/queue.h
    ell/random.h
    ell/ringbuf.h
    ell/settings.h
    ell/signal.h
    ell/string.h
    ell/strv.h
    ell/test.h
    ell/time.h
    ell/timeout.h
    ell/tls.h
    ell/uintset.h
    ell/utf8.h
    ell/util.h
    ell/uuid.h
    TYPE INCLUDE
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/ell.pc" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

set(UNIT_TESTS
    unit/test-unit
    unit/test-queue
    unit/test-hashmap
    unit/test-endian
    unit/test-string
    unit/test-utf8
    unit/test-main
    unit/test-io
    unit/test-ringbuf
    unit/test-plugin
    unit/test-checksum
    unit/test-settings
    unit/test-netlink
    unit/test-genl-msg
    unit/test-siphash
    unit/test-cipher
    unit/test-random
    unit/test-util
    unit/test-uintset
    unit/test-base64
    unit/test-uuid
    unit/test-pbkdf2
    unit/test-dhcp
    unit/test-dir-watch
    unit/test-ecc
    unit/test-ecdh
    unit/test-time
)

set(DBUS_TESTS
    unit/test-hwdb
    unit/test-dbus
    unit/test-dbus-util
    unit/test-dbus-message
    unit/test-dbus-message-fds
    unit/test-dbus-service
    unit/test-dbus-watch
    unit/test-dbus-properties
    unit/test-gvariant-util
    unit/test-gvariant-message
)

set(CERT_FILES
    unit/cert-chain.pem
    unit/cert-entity-int.pem
    unit/cert-server.pem
    unit/cert-client.pem
    unit/cert-server-key-pkcs8.pem
    unit/cert-client-key-pkcs8.pem
    unit/cert-client-key-md5-des.pem
    unit/cert-client-key-sha1-des.pem
    unit/cert-client-key-v2-des.pem
    unit/cert-client-key-v2-des-ede3.pem
    unit/cert-client-key-v2-aes128.pem
    unit/cert-client-key-v2-aes256.pem
)

set(CERT_CHECKS
    unit/cert-intca
    unit/cert-entity-int
    unit/cert-server
    unit/cert-client
)

set(EXAMPLES
    examples/dbus-service
    examples/https-client-test
    examples/https-server-test
    examples/dbus-client
    examples/dhcp-client
)

if(ENABLE_GLIB)
    list(APPEND EXAMPLES examples/glib-eventloop)
    target_link_options(examples/glib-eventloop "${GLIB_LIBS}")
endif()

list(APPEND TESTS ${UNIT_TESTS})
if(ENABLE_DBUS_TESTS)
    list(APPEND TESTS ${DBUS_TESTS})
endif()
if(ENABLE_CERT_TESTS)
    list(APPEND TESTS ${CERT_TESTS})
endif()

