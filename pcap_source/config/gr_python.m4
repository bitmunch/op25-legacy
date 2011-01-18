dnl
dnl Copyright 2003,2004,2005 Free Software Foundation, Inc.
dnl 
dnl This file is part of OP25
dnl 
dnl OP25 is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 3, or (at your option)
dnl any later version.
dnl 
dnl OP25 is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl 
dnl You should have received a copy of the GNU General Public License
dnl along with OP25; see the file COPYING.  If not, write to
dnl the Free Software Foundation, Inc., 51 Franklin Street,
dnl Boston, MA 02110-1301, USA.
dnl 

# PYTHON_DEVEL()
#
# Checks for Python and tries to get the include path to 'Python.h'.
# It provides the $(PYTHON_CPPFLAGS) and $(PYTHON_LDFLAGS) output variables.
#
AC_DEFUN([PYTHON_DEVEL],[
	AC_REQUIRE([AM_PATH_PYTHON])
	AC_REQUIRE([AC_CANONICAL_HOST])

	# For Fedora Core 5 and 6, see ticket:39 in Trac
	if test -f '/etc/redhat-release'; then
		if  (echo $pyexecdir | grep -q lib64); then
			pythondir="$pyexecdir"
		fi
	fi

	# Check for Python include path
	AC_MSG_CHECKING([for Python include path])
	if test -z "$PYTHON" ; then
		AC_MSG_ERROR([cannot find Python path])
	fi

	# ask distutils which include path we should use
	python_cmd='
import distutils.sysconfig
import os
path = distutils.sysconfig.get_python_inc(plat_specific=False)
if os.sep == "\\":
  path = path.replace("\\", "/")
print path
'
	python_path=`$PYTHON -c "$python_cmd"`
	AC_MSG_RESULT([$python_path])
	if test -z "$python_path" ; then
		AC_MSG_ERROR([cannot find Python include path])
	fi

	AC_SUBST(PYTHON_CPPFLAGS,[-I$python_path])

	# Check for Python headers usability
	python_save_CPPFLAGS=$CPPFLAGS
	CPPFLAGS="$CPPFLAGS $PYTHON_CPPFLAGS"
	AC_CHECK_HEADERS([Python.h], [],
			[AC_MSG_ERROR([cannot find usable Python headers])])
	CPPFLAGS="$python_save_CPPFLAGS"

	# Only set this on mingw and cygwin hosts, (only implemented
	# for mingw host, for crosscompiling you need to trick this)

	PYTHON_LDFLAGS=""
	case $host_os in
	     *mingw* | *cygwin* )
	  AC_MSG_CHECKING([for Python LDFLAGS])

        python_cmd='
import distutils.sysconfig
import os
path = distutils.sysconfig.get_config_var("LIBPL")
if path == None:
  path = distutils.sysconfig.PREFIX + "/libs"
if os.sep == "\\":
  path = path.replace("\\", "/")
print path
'      
	  python_stdlib_path=`$PYTHON -c "$python_cmd"`

	  python_version_nodot=`echo $PYTHON_VERSION | sed "s,\.,,"`
	  libpython_name="python$PYTHON_VERSION"

	  # Standard install of python for win32 has libpython24.a
	  # instead of libpython2.4.a so we check for the library
	  # without the dot in the version number.

	  python_stdlib_filename=`find $python_stdlib_path -type f -name libpython$python_version_nodot.* -print | sed "1q"`
	  if test -n "$python_stdlib_filename" ; then
	  	libpython_name="python$python_version_nodot"
	  fi

          PYTHON_LDFLAGS="-L$python_stdlib_path -l$libpython_name"
          AC_MSG_RESULT($PYTHON_LDFLAGS) 
          # Replace all backslashes in PYTHON Paths with forward slashes
          pythondir=`echo $pythondir |sed 's,\\\\,/,g'`
          pkgpythondir=`echo $pkgpythondir |sed 's,\\\\,/,g'`
          pyexecdir=`echo $pyexecdir |sed 's,\\\\,/,g'`
          pkgpyexecdir=`echo $pkgpyexecdir |sed 's,\\\\,/,g'`
	  ;;
        esac

	AC_SUBST([PYTHON_LDFLAGS])
])