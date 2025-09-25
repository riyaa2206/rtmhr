check_include_file_cxx(stdint.h HAVE_STDINT_H)
if(HAVE_STDINT_H)
    add_definitions(-DHAVE_STDINT_H)
endif()

set(examples_as_tests_sources)
if(${ENABLE_EXAMPLES})
    set(examples_as_tests_sources
        #test/rtmhr-examples-test-suite.cc
        )
endif()

build_lib(
    LIBNAME rtmhr
    SOURCE_FILES model/rtmhr.cc
                 model/rtmhr-impl.cc
                 helper/rtmhr-helper.cc
    HEADER_FILES model/rtmhr.h
                 helper/rtmhr-helper.h
    LIBRARIES_TO_LINK ${libcore}
                      ${libnetwork}
                      ${libinternet}
                      ${libwifi}
                      ${libmobility}
    TEST_SOURCES test/rtmhr-test-suite.cc
                 ${examples_as_tests_sources}
)
