/* -*- C++ -*- */

/*
 * Copyright 2008-2009 Steve Glass
 *
 * This file is part of OP25.
 *
 * OP25 is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or(at your option)
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

#ifndef INCLUDED_IMBE_H
#define INCLUDED_IMBE_H

class op25_imbe {
public:
	op25_imbe();
	~op25_imbe();
	void imbe_frame(unsigned char*);
	void init();

private:
// define global lookup tables

//NOTE: Single-letter variable names are upper case only;
//            Lower case if needed is spelled.   e.g.  L, ell

// global Seq ER ?

//Working arrays
	int bee[58];         //Encoded Spectral Amplitudes
	float M[57][2];   //Enhanced Spectral Amplitudes
	float Mu[57][2];  //Unenhanced Spectral Amplitudes
	int vee[57][2]; //V/UV decisions
	float suv[160];        //Unvoiced samples
	float sv[160];         //Voiced samples
	float log2Mu[58][2];
	float Olduw[256];
	float psi1;
	float phi[57][2];

//Variables
	int Old;
	int New;
	int L;
	int OldL;
	float w0;
	float Oldw0;
	float Luv; //number of unvoiced spectral amplitudes
	int BOT;

	char sym_b[4096];
	char RxData[4096];
	int sym_bp;
	int ErFlag;

   FILE *out;

// member functions
	uint32_t extract(const uint8_t* buf, size_t begin, size_t end);

	unsigned int vfPickBits0(unsigned char *);
	unsigned int vfPickBits1(unsigned char *);
	unsigned int vfPickBits2(unsigned char *);
	unsigned int vfPickBits3(unsigned char *);
	unsigned int vfPickBits4(unsigned char *);
	unsigned int vfPickBits5(unsigned char *);
	unsigned int vfPickBits6(unsigned char *);
	unsigned int vfPickBits7(unsigned char *);

	int vfHmg15113Dec(int , int );
	unsigned int gly23127Dec(unsigned int& , unsigned int& );
	unsigned int  gly23127GetSyn(unsigned int );

	unsigned int vfPrGen15(unsigned int& );
	unsigned int vfPrGen23(unsigned int& );

#if 0
	void vfDec(const uint8_t*, unsigned int&, unsigned int&, unsigned int&, unsigned int&, unsigned int&, unsigned int&, unsigned int&, unsigned int&, unsigned int&, unsigned int&);
#else 
	void vfDec(unsigned char*, unsigned int&, unsigned int&, unsigned int&, unsigned int&, unsigned int&, unsigned int&, unsigned int&, unsigned int&, unsigned int&, unsigned int&);
#endif
	void decode(uint8_t *);
	void DecodeSpectralAmplitudes(int, int );
	void DecodeVUV(int );
	void AdaptiveSmoothing(float, float, float );
	void CplxFFT(float [], float []);
	void EnhanceSpectralAmplitudes(float&);
	void RealIFFT(float [], float [], float []);
	uint16_t rearrange(uint32_t u0, uint32_t u1, uint32_t u2, uint32_t u3, uint32_t u4, uint32_t u5, uint32_t u6, uint32_t u7);
	void SynthUnvoiced(void);
	void SynthVoiced(void);
	void Synth(void);
	void unpack(uint8_t *buf, uint32_t& u0, uint32_t& u1, uint32_t& u2, uint32_t& u3, uint32_t& u4, uint32_t& u5, uint32_t& u6, uint32_t& u7, uint32_t& E0, uint32_t& ET);
};


#endif /* INCLUDED_IMBE_H */
