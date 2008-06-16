/* -*- C++ -*- */
/*
 * Copyright 2008 Steve Glass
 * 
 * This file is part of OP25.
 * 
 * OP25 is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * OP25 is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with OP25; see the file COPYING. If not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Boston, MA
 * 02110-1301, USA.
 */

#ifndef INCLUDED_BCH_H
#define INCLUDED_BCH_H

#include <stdint.h>

/*
 * BCH encode val and return the result.
 */
extern uint64_t bch_64_encode(uint16_t val);

/*
 * BCH decode codeword cw and correct errors. If successful, the cw is
 * error-corrected and  true is returned; otherwise returns false.
 */
extern bool bch_64_decode(uint64_t& cw);

#endif /* INCLUDED_BCH_H */
