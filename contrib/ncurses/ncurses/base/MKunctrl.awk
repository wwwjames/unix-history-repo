# $Id: MKunctrl.awk,v 1.11 2005/12/17 22:48:37 tom Exp $
##############################################################################
# Copyright (c) 1998-2004,2005 Free Software Foundation, Inc.                #
#                                                                            #
# Permission is hereby granted, free of charge, to any person obtaining a    #
# copy of this software and associated documentation files (the "Software"), #
# to deal in the Software without restriction, including without limitation  #
# the rights to use, copy, modify, merge, publish, distribute, distribute    #
# with modifications, sublicense, and/or sell copies of the Software, and to #
# permit persons to whom the Software is furnished to do so, subject to the  #
# following conditions:                                                      #
#                                                                            #
# The above copyright notice and this permission notice shall be included in #
# all copies or substantial portions of the Software.                        #
#                                                                            #
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR #
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   #
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    #
# THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      #
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    #
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        #
# DEALINGS IN THE SOFTWARE.                                                  #
#                                                                            #
# Except as contained in this notice, the name(s) of the above copyright     #
# holders shall not be used in advertising or otherwise to promote the sale, #
# use or other dealings in this Software without prior written               #
# authorization.                                                             #
##############################################################################
#
# Author: Thomas E. Dickey <dickey@clark.net> 1997
#

BEGIN	{
		print "/* generated by MKunctrl.awk */"
		print ""
		print "#include <curses.priv.h>"
		print ""
		print "#undef unctrl"
		print ""
	}
END	{
		print "NCURSES_EXPORT(NCURSES_CONST char *) unctrl (register chtype ch)"
		print "{"

		printf "static const char* const table[] = {"
		for ( ch = 0; ch < 256; ch++ ) {
			gap = ","
			if ((ch % 8) == 0)
				printf "\n    "
			if (ch < 32) {
				printf "\"^\\%03o\"", ch + 64
			} else if (ch == 127) {
				printf "\"^?\""
			} else if (ch >= 128 && ch < 160) {
				printf "\"~\\%03o\"", ch - 64
			} else {
				printf "\"\\%03o\"", ch
				gap = gap " "
			}
			if (ch == 255)
				gap = "\n"
			else if (((ch + 1) % 8) != 0)
				gap = gap " "
			printf "%s", gap
		}
		print "};"

		print ""
		print "#if NCURSES_EXT_FUNCS"
		printf "static const char* const table2[] = {"
		for ( ch = 128; ch < 160; ch++ ) {
			gap = ","
			if ((ch % 8) == 0)
				printf "\n    "
			if (ch >= 128 && ch < 160) {
				printf "\"\\%03o\"", ch
				gap = gap " "
			}
			if (ch == 255)
				gap = "\n"
			else if (((ch + 1) % 8) != 0)
				gap = gap " "
			printf "%s", gap
		}
		print "};"
		print "#endif /* NCURSES_EXT_FUNCS */"

		print ""
		print "\tint check = ChCharOf(ch);"
		print "\tconst char *result;"
		print ""
		print "\tif (check >= 0 && check < (int)SIZEOF(table)) {"
		print "#if NCURSES_EXT_FUNCS"
		print "\t\tif ((SP != 0)"
		print "\t\t && (SP->_legacy_coding > 1)"
		print "\t\t && (check >= 128)"
		print "\t\t && (check < 160))"
		print "\t\t\tresult = table2[check - 128];"
		print "\t\telse"
		print "#endif /* NCURSES_EXT_FUNCS */"
		print "\t\t\tresult = table[check];"
		print "\t} else {"
		print "\t\tresult = 0;"
		print "\t}"
		print "\treturn (NCURSES_CONST char *)result;"
		print "}"
	}
