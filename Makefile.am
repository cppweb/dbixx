noinst_PROGRAMS = test
test_SOURCES = test.cpp 

test_LDADD = libdbixx.la
test_CXXFLAGS = -Winvalid-pch -Wall

lib_LTLIBRARIES     = libdbixx.la
libdbixx_la_SOURCES = row.cpp session.cpp result.cpp
libdbixx_la_CXXFLAGS =-Winvalid-pch -Wall

nobase_pkginclude_HEADERS = dbixx.h

PCH_SOURCE=dbixx.h
PCH_FILE=$(PCH_SOURCE).gch

$(libdbixx_la_OBJECTS) $(test_OBJECTS): $(PCH_FILE)

sinclude ./dbi.h.gch.dep

$(cppblog_OBJECTS): $(PCH_FILE)

$(PCH_FILE):
	$(CXXCOMPILE) $(PCH_SOURCE) -o $(PCH_FILE) -MT $(PCH_FILE) -MD -MP -MF dbixx.h.gch.dep

clean-local:
	rm -f dbixx.h.gch.dep $(PCH_FILE)

