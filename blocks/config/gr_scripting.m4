dnl
dnl Copyright 2003 Free Software Foundation, Inc.
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

AC_DEFUN([GR_SCRIPTING],[
  AC_REQUIRE([AC_PROG_LN_S])
  AC_REQUIRE([AC_PROG_CXX])
  AC_REQUIRE([AC_PROG_LIBTOOL])

  SWIG_PROG(1.3.23)
  SWIG_ENABLE_CXX
  SWIG_PYTHON
])
