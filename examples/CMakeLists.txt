build_lib_example(
    NAME rtmhr-example
    SOURCE_FILES rtmhr-example.cc
    LIBRARIES_TO_LINK ${librtmhr}
                     ${libapplications}
                     ${libflow-monitor}
                     ${libinternet-apps}
)

build_lib_example(
    NAME quick-evaluation
    SOURCE_FILES quick_evaluation.cc
    LIBRARIES_TO_LINK ${librtmhr}
                     ${libapplications}
                     ${libaodv}
                     ${libflow-monitor}
                     ${libinternet-apps}
)

build_lib_example(
    NAME simple-evaluation
    SOURCE_FILES simple_evaluation.cc
    LIBRARIES_TO_LINK ${librtmhr}
                     ${libapplications}
                     ${libflow-monitor}
                     ${libinternet-apps}
)

build_lib_example(
    NAME comparative-evaluation
    SOURCE_FILES comparative_evaluation.cc
    LIBRARIES_TO_LINK ${librtmhr}
                     ${libaodv}
                     ${libapplications}
                     ${libflow-monitor}
                     ${libinternet-apps}
)
