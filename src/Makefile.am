bin_PROGRAMS = galacticd

galacticd_SOURCES = galacticd.c galacticd.h \
                    scoreboard.c scoreboard.h \
                    common.c common.h \
                    QRBG/QRBG.cpp QRBG/QRBG.h \
                    QRBG/QRBG_wrapper.cpp QRBG/QRBG_wrapper.h

galacticd_CFLAGS = @SQLITE3_CFLAGS@
galacticd_LDFLAGS = @SQLITE3_LIBS@
