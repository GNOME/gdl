dnl This was taken from evolution's configure.in and modified a bit.
AC_DEFUN(GDL_CHECK_LIB, [
	dispname="$1"
	pkgname="$2"
	minvers="$3"
	maxvers="$4"

	AC_MSG_CHECKING(for $dispname)

	if gnome-config --libs $pkgname > /dev/null 2>&1; then
		pkgvers=`gnome-config --modversion $pkgname | sed -e 's/^[[^0-9]]*//'`
	else
		pkgvers=not
	fi
	AC_MSG_RESULT($pkgvers found)

	pkgvers=`echo $pkgvers | awk -F. '{ print $[]1 * 1000000 + $[]2 * 10000 + $[]3 * 100 + $[]4;}'`
	cmpminvers=`echo $minvers | awk -F. '{ print $[]1 * 1000000 + $[]2 * 10000 + $[]3 * 100 + $[]4;}'`
	cmpmaxvers=`echo $maxvers | awk -F. '{ print $[]1 * 1000000 + $[]2 * 10000 + $[]3 * 100 + $[]4;}'`
	ok=yes
	if test "$pkgvers" -lt $cmpminvers; then
		ok=no
	elif test -n "$maxvers" -a "$pkgvers" -ge $cmpmaxvers; then
		ok=no
	fi
	if test $ok = no; then
		case $maxvers in
		"")
			dispvers="$minvers or higher"
			;;
		$minvers)
			dispvers="$minvers (exactly)"
			;;
		*)
			dispvers="$minvers or higher, but less than $maxvers,"
			;;
		esac

		AC_MSG_ERROR([
""
"You need $dispname $dispvers to build GDL"
"If you think you already have this installed, consult the README."])
	fi

	tmp_bsnom=`echo $pkgname | tr a-z A-Z`
	eval $tmp_bsnom'_CFLAGS'=\"`gnome-config --cflags $pkgname`\"
	eval $tmp_bsnom'_LIBS'=\"`gnome-config --libs $pkgname`\"
])