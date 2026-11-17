# cmake -DSOURCE=<checkout> -DOUT=<header> -P version.cmake: the checkout's commit as FNVD_CRASHLOG_COMMIT for the
# crash log. The header is rewritten only when the commit changes, so only the capture object recompiles, and only
# then; the link time comes from the image's own time stamp.
execute_process(COMMAND git -C "${SOURCE}" describe --always --dirty --abbrev=10
        OUTPUT_VARIABLE commit OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET RESULT_VARIABLE failed)
if (failed OR commit STREQUAL "")
    set(commit unknown)
endif ()
file(CONFIGURE OUTPUT "${OUT}" CONTENT "#define FNVD_CRASHLOG_COMMIT \"@commit@\"\n" @ONLY)
