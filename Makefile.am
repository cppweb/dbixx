noinst_PROGRAMS = test
test_SOURCES = test.cpp
test_LDADD = libdbixx.la
test_CXXFLAGS = -Wall

lib_LTLIBRARIES     = libdbixx.la

libdbixx_la_SOURCES = row.cpp session.cpp result.cpp
libdbixx_la_LDFLAGS  = -version-info 1:0:0 -no-undefined
libdbixx_la_CXXFLAGS = -Wall

nobase_pkginclude_HEADERS = dbixx.h
