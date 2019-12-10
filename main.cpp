#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "math.h"
#include "stdint.h"


const int32_t MAXTEXT=(1 << 12);
const int64_t INTMAX=(( (int64_t)1 << 32 ) -1);
const int32_t MAXTERME=1024;

enum { NUMERAL_UNDEF=0,NUMERAL_VAR=1,NUMERAL_MUL,NUMERAL_ZAHL,NUMERAL_POT };
enum { FUNC_Z2C=1,FUNC_Z3AZC,FUNC_Z4AZC,FUNC_Z5AZC,
	FUNC_TRICZ4B,FUNC_Z6AZC,FUNC_USER,
	FUNCANZ
};

const char funcname[][32] = {
	"(undef)","Z2C","Z3AZC","Z4AZC","Z5AZC","TRICZ4B",
	"Z6AZC","(user)"
};

const int32_t VARX='x'-'a';
const int32_t VARY='y'-'a';
const int32_t VARZ='z'-'a';

// Masterclass

struct Numeral { 
	int32_t id;

	Numeral() { id=NUMERAL_UNDEF; }

	virtual char* getText(char*) { exit(99); }
	virtual double evalMax(void) { exit(99); };
	virtual int32_t fracBits(void) { exit(99); };
	virtual char* textFrac(char*) { exit(99); };
};

typedef Numeral *PNumeral;

struct NumeralZahl : public Numeral {
	double w;
	
	NumeralZahl() { id=NUMERAL_ZAHL; w=1.0; }
	
	virtual char* getText(char*);
	virtual double evalMax(void);
	virtual int32_t fracBits(void);
	virtual char* textFrac(char*);
};

struct NumeralVar : public Numeral {
	int32_t varidx;

	NumeralVar() { id=NUMERAL_VAR; varidx=-1; }
	
	virtual char* getText(char*);
	virtual double evalMax(void);
	virtual int32_t fracBits(void);
	virtual char* textFrac(char*);
};

struct NumeralMul : public Numeral {
	Numeral *l,*r;
	
	NumeralMul() { id=NUMERAL_MUL; l=r=NULL; }

	virtual char* getText(char*);
	virtual double evalMax(void);
	virtual int32_t fracBits(void);
	virtual char* textFrac(char*);
};

struct NumeralPot : public NumeralMul {
	NumeralPot() { id=NUMERAL_POT; l=r=NULL; }

	virtual char* getText(char*);
	virtual double evalMax(void);
	virtual int32_t fracBits(void);
	virtual char* textFrac(char*);
};

// certificate of analysis
struct ZertifikatTerme {
	int32_t anz;
	char* termstr;
	PNumeral pterme[MAXTERME];
	
	ZertifikatTerme();
	virtual ~ZertifikatTerme();
	void clearAll(void);
	void addTerm(const char*);
	char* textFrac(char*);
	void setTerme(const char*);

	// main routine
	void termTab(FILE*);
};

struct ZertifikatVar {
	double maxw[26]; // indiziert über ASCII-65
	int32_t fracbits[26];
	char used[26];
	int32_t exponentAxisRange;
	int32_t refinement;
	
	ZertifikatVar();
	void clearVar(void);
	void setValueByChar(const char,const double);
	void setValueByIdx(const int32_t,const double);
	void setFracBitsByIdx(const int32_t,const int32_t);
	
	double getValueByIdx(const int32_t);
	double getValueByChar(const char);
	int32_t getFracBitsByIdx(const int32_t);
};


// globals

int32_t BASEDENOM=25;
int32_t FUNC=FUNC_Z2C;
ZertifikatTerme zt_real,zt_imag,zt_pz,zt_user;
ZertifikatVar zt_var;
char funcuserstr[1024];
int32_t _R=0;


// forward

Numeral* minimalparseBitPrec_stripped(const char* expr);


// functions

int32_t bitsDec(const int64_t aw) {
	long double w;
	if (aw < 0) w=-aw; else w=aw;
	
	// +1: to get 3 bits for the value 4 and not only 2
	return (int)ceil(
		log(1.0+w) / log(2.0)
	);
}

char* upper(char* s) {
	if (!s) return NULL;
	for(uint32_t i=0;i<strlen(s);i++) {
		if ( (s[i]>='a') && (s[i]<='z') ) s[i]=s[i]-'a'+'A';
	}

	return s;
}

char* lower(char* s) {
	if (!s) return NULL;
	for(uint32_t i=0;i<strlen(s);i++) {
		if ( (s[i]>='A') && (s[i]<='Z') ) s[i]=s[i]-'A'+'a';
	}

	return s;
}

char lochar(const char ch) {
	if ((ch >= 'A') && (ch <= 'Z')) return (ch-'A'+'a');
	
	return ch;
}

int getVarIdx(const char ch) {
	char erg=lochar(ch);
	if ( (erg >= 'a') && (erg <= 'z') ) return (erg-'a');
	return -1;
}

// numeralVar

char* NumeralVar::getText(char* erg) {
	sprintf(erg,"%c",varidx+'a');
	return erg;
}

int32_t NumeralVar::fracBits(void) { 
	if (
		(varidx == VARX) ||
		(varidx == VARY) ||
		(varidx == VARZ)
	) {
		// dependent on the refinement level
		return (zt_var.refinement-zt_var.exponentAxisRange);
	}
		
	return zt_var.getFracBitsByIdx(varidx);
}

char* NumeralVar::textFrac(char* erg) { 
	switch (varidx) {
		case VARX: case VARY: case VARZ:
			sprintf(erg,".(SCALE)");
			break;
		default:
			sprintf(erg,".%i",fracBits()); 
			break;
	}
	
	return erg;
}

double NumeralVar::evalMax(void) {
	return zt_var.getValueByIdx(varidx);
}

// numeralZahl

char* NumeralZahl::getText(char* erg) {
	sprintf(erg,"%.20lg",w);
	return erg;
}

int32_t NumeralZahl::fracBits(void) {
	// da Integer => 0
	return 0;
}

char* NumeralZahl::textFrac(char* erg) {
	sprintf(erg,".0");
	return erg;
}

double NumeralZahl::evalMax(void) {
	return w;
}

// numeralMul

char* NumeralMul::getText(char* erg) {
	char t1[MAXTEXT],t2[MAXTEXT],t3[MAXTEXT],t4[MAXTEXT];
	if ( 
		(l->id==NUMERAL_ZAHL) ||
		(l->id==NUMERAL_VAR)
	) l->getText(t1);
	else sprintf(t1,"(%s)",l->getText(t3));
	
	if ( 
		(r->id==NUMERAL_ZAHL) ||
		(r->id==NUMERAL_VAR)
	) r->getText(t2);
	else sprintf(t2,"(%s)",r->getText(t4));

	sprintf(erg,"%s*%s",t1,t2);
	return erg;
}

int32_t NumeralMul::fracBits(void) {
	return ( l->fracBits() + r->fracBits() );
}

char* NumeralMul::textFrac(char* erg) {
	char* t1=new char[MAXTEXT];
	char* t2=new char[MAXTEXT];
	
	sprintf(erg,"(%s)+(%s)",l->textFrac(t1),r->textFrac(t2));
	
	delete[] t1;
	delete[] t2;
	
	return erg;
}

double NumeralMul::evalMax(void) {
	return (l->evalMax() * r->evalMax());
}

// numeralPot

char* NumeralPot::getText(char* erg) {
	char t1[MAXTEXT],t2[MAXTEXT],t3[MAXTEXT],t4[MAXTEXT];
	if ( 
		(l->id==NUMERAL_ZAHL) ||
		(l->id==NUMERAL_VAR)
	) l->getText(t1);
	else sprintf(t1,"(%s)",l->getText(t3));
	
	if ( 
		(r->id==NUMERAL_ZAHL) ||
		(r->id==NUMERAL_VAR)
	) r->getText(t2);
	else sprintf(t2,"(%s)",r->getText(t4));

	sprintf(erg,"%s^%s",t1,t2);
	
	return erg;
}

int32_t NumeralPot::fracBits(void) {
	return ( (l->fracBits()) * r->evalMax() );
}

char* NumeralPot::textFrac(char* erg) {
	char *t1=new char[MAXTEXT];
	sprintf(erg,"%.0lf*(%s)",ceil(r->evalMax()),l->textFrac(t1));
	
	delete[] t1;
	return erg;
}

double NumeralPot::evalMax(void) {
	int32_t expo=(int)ceil(r->evalMax());
	double basis=l->evalMax();
	double w=1.0;
	for(int32_t i=1;i<=expo;i++) {
		w *= basis;
		if (w > INTMAX) {
			printf("Overflow.\n");
			exit(99);
		}
	}
	
	return w;
}


// allocate objects

Numeral* newNumeralZahl(const double a) {
	NumeralZahl *p=new NumeralZahl;
	p->w=a;
	return p;
}

Numeral* newNumeralVar(const char s) {
	NumeralVar *p=new NumeralVar;
	p->varidx=getVarIdx(lochar(s));
	return p;
}

Numeral* newNumeralMul(Numeral* al,Numeral* ar) {
	NumeralMul *p=new NumeralMul;
	p->l=al;
	p->r=ar;
	return p;
}

Numeral* newNumeralPot(Numeral* al,Numeral* ar) {
	NumeralPot *p=new NumeralPot;
	p->l=al;
	p->r=ar;
	return p;
}

Numeral* minimalparseBitPrec_stripped(const char* expr) {
	char *p=strchr(expr,'*'); // gibt ja keine Plusse mehr
	char* l=new char[MAXTEXT];
	char* r=new char[MAXTEXT];
	if (p) {
		strcpy(l,expr);	l[p-expr]=0;
		strcpy(r,p+1);
		Numeral *pl=minimalparseBitPrec_stripped(l);
		Numeral *pr=minimalparseBitPrec_stripped(r);
		
		delete[] l;
		delete[] r;
		return newNumeralMul(pl,pr);
	} // p
	
	// terms do not have any +/-
	
	p=strchr(expr,'^'); 
	if (p) {
		strcpy(l,expr);	l[p-expr]=0;
		strcpy(r,p+1);
		Numeral *pl=minimalparseBitPrec_stripped(l);
		Numeral *pr=minimalparseBitPrec_stripped(r);
		
		delete[] l;
		delete[] r;
		return newNumeralPot(pl,pr);
	} // p
	
	delete[] l;
	delete[] r;

	// is a digit or a variable
	if (strlen(expr) != 1) {
		printf("Error parser: |%s|\n",expr);
		exit(8);
	}
	
	if ( (expr[0]>='0')&&(expr[0]<='9') ) {
		return newNumeralZahl(expr[0]-'0');
	}
	
	if (strlen(expr) == 1) {
		// Variable
		return newNumeralVar(expr[0]);
	}

	printf("unknown term |%s|",expr);

	return NULL;
}

void formel_z3azc(const int bits,const double range) {
	zt_var.exponentAxisRange=bits;
		
	zt_var.setValueByChar('x',range);
	zt_var.setValueByChar('y',range);
		
	char tmp[1024];
	sprintf(tmp,"_zertifikat.z3azc_bit.precision_R%.20lg.txt",range);
	FILE *f=fopen(tmp,"wt");
	fprintf(f,"z^3+A*z+c == (x+i*y)^3 + (f+g*i)*(x+i*y) + (d+e*i)\n\n");
	
	fprintf(f,"|re(C)=d|<=%.20lg,|im(C)=e|<=%.20lg\n",
		zt_var.getValueByChar('d'),
		zt_var.getValueByChar('e')
	);
	fprintf(f,"|re(A)=f|<=%.20lg,|im(A)=g|<=%.20lg\n",
		zt_var.getValueByChar('f'),
		zt_var.getValueByChar('g')
	);
	fprintf(f,"|x|<=%.20lg,|y|<=%.20lg\n",
		zt_var.getValueByChar('x'),
		zt_var.getValueByChar('y')
	);
	
	fprintf(f,"\naxisrange = -%.20lg .. +%.20lg\n",-range,+range);
	fprintf(f,"SCALE := (2*axis range) / (2^REFINEMENT LEVEL == pixels per axis)\n");
	fprintf(f,"  := 2^-(REF-%i)",zt_var.exponentAxisRange);
		
	fprintf(f,"\n\nnotation: e.g. 3.21 = 24\n");
	fprintf(f,"  3 bits for the integer part\n");
	fprintf(f," 21 bits for the fractional part\n");
	fprintf(f," 24 total bits\n\n");

	fprintf(f,"\n===============================================================\n|real z_new| <=\n");
	zt_real.termTab(f);
		
	fprintf(f,"\n===============================================================\n|imag z_new| <=\n");
	zt_imag.termTab(f);

	fclose(f);
}

void formel_user(const int bits,const double range) {
	zt_var.exponentAxisRange=bits;
	
	zt_var.setValueByChar('x',range);
	zt_var.setValueByChar('y',range);
		
	char tmp[1024];
	sprintf(tmp,"_zertifikat.user_bit.precision_R%.20lg.txt",range);
	FILE *f=fopen(tmp,"wt");
	fprintf(f,"%s\n",zt_user.termstr);
	
	for(int idx=0;idx<26;idx++) {
		if (zt_var.used[idx]>0) {
			fprintf(f,"|%c|<=%.20lg, max fracBits=%i\n",
				'a'+idx,
				zt_var.getValueByIdx(idx),
				zt_var.getFracBitsByIdx(idx)
			);
		}
	}
	
	fprintf(f,"\naxisrange = -%.20lg .. +%.20lg\n",-range,+range);
	fprintf(f,"SCALE := (2*axis range) / (2^REFINEMENT LEVEL == pixels per axis)\n");
	fprintf(f,"  := 2^-(REF-%i)",zt_var.exponentAxisRange);

	fprintf(f,"\n\nnotation: e.g. 3.21 = 24\n");
	fprintf(f,"  3 bits for the integer part\n");
	fprintf(f," 21 bits for the fractional part\n");
	fprintf(f," 24 total bits\n\n");

	fprintf(f,"\n===============================================================\n|f_new| <=\n");
	zt_user.termTab(f);
		
	fclose(f);
}

void formel_z3azc(void) {
	zt_real.setTerme("d +  f *x - g *y + x^3 - 3 *x *y^2");
	zt_imag.setTerme("e + f *y + g *x + 3 *x^2 *y - y^3");
	
	zt_var.clearVar();
	zt_var.setValueByChar('d',2);
	zt_var.setValueByChar('e',2);
	zt_var.setValueByChar('f',2);
	zt_var.setValueByChar('g',2);
	
	if (_R>0) {
		formel_z3azc(
			(int)ceil(log(2.0*_R) / log(2.0)),
			_R
		);
	} else {
		for(int32_t bb=1;bb<=3;bb++) {
			formel_z3azc(
				bb+1,
				(1 << bb)
			);
		}
	}
}

void formel_user(const char* as) {
	zt_user.setTerme(as);
	
	// zt_var variables set outside
	
	if (_R>0) {
		formel_user(
			(int)ceil(log(2.0*_R) / log(2.0)),
			_R
		);
	} else {
		for(int32_t bb=1;bb<=3;bb++) {
			formel_user(
				bb+1,
				(1 << bb)
			);
		}
	}
}

void formel_z2c(const int bits,const double range) {
	zt_var.exponentAxisRange=bits;
		
	zt_var.setValueByChar('x',range);
	zt_var.setValueByChar('y',range);
		
	char tmp[1024];
	sprintf(tmp,"_zertifikat.z2c_bit.precision_R%.20lg.txt",range);
	FILE *f=fopen(tmp,"wt");
	fprintf(f,"z^2+c == (x+i*y)^2 + (d+e*i)\n\n");
	
	fprintf(f,"|re(C)=d|<=%.20lg,|im(C)=e|<=%.20lg\n",
		zt_var.getValueByChar('d'),
		zt_var.getValueByChar('e')
	);
	fprintf(f,"|x|<=%.20lg,|y|<=%.20lg\n",
		zt_var.getValueByChar('x'),
		zt_var.getValueByChar('y')
	);

	fprintf(f,"SCALE := (2*axis range) / (2^REFINEMENT LEVEL == pixels per axis)\n");
	fprintf(f,"  := 2^-(REF-%i)",zt_var.exponentAxisRange);

	fprintf(f,"\naxisrange = -%.20lg .. +%.20lg\n",-range,+range);
		
	fprintf(f,"\n\nnotation: e.g. 3.21 = 24\n");
	fprintf(f,"  3 bits for the integer part\n");
	fprintf(f," 21 bits for the fractional part\n");
	fprintf(f," 24 total bits\n\n");

	fprintf(f,"\n===============================================================\n|real z_new| <=\n");
	zt_real.termTab(f);
		
	fprintf(f,"\n===============================================================\n|imag z_new| <=\n");
	zt_imag.termTab(f);

	fclose(f);
}

void formel_z2c(void) {
	zt_real.setTerme("d + x^2  - y^2");
	zt_imag.setTerme("e + 2* x* y");
	
	zt_var.clearVar();
	zt_var.setValueByChar('d',2);
	zt_var.setValueByChar('e',2);
	
	if (_R>0) {
		formel_z2c(
			(int32_t)ceil(log(2.0*_R) / log(2.0)),
			_R
		);
	} else {
		for(int32_t bb=1;bb<=3;bb++) {
			formel_z2c(
				bb+1,
				(1 << bb)
			);
		}
	}
};

void formel_z4azc(const int bits,const double range) {
	zt_var.exponentAxisRange=bits;
		
	zt_var.setValueByChar('x',range);
	zt_var.setValueByChar('y',range);
		
	char tmp[1024];
	sprintf(tmp,"_zertifikat.z4azc_bit.precision_R%.20lg.txt",range);
	FILE *f=fopen(tmp,"wt");
	fprintf(f,"z^4+A*z+c == (x+i*y)^4 + (f+g*i)*(x+i*y) + (d+e*i)\n\n");
	
	fprintf(f,"|re(C)=d|<=%.20lg,|im(C)=e|<=%.20lg\n",
		zt_var.getValueByChar('d'),
		zt_var.getValueByChar('e')
	);
	fprintf(f,"|re(A)=f|<=%.20lg,|im(A)=g|<=%.20lg\n",
		zt_var.getValueByChar('f'),
		zt_var.getValueByChar('g')
	);
	fprintf(f,"|x|<=%.20lg,|y|<=%.20lg\n",
		zt_var.getValueByChar('x'),
		zt_var.getValueByChar('y')
	);

	fprintf(f,"SCALE := (2*axis range) / (2^REFINEMENT LEVEL == pixels per axis)\n");
	fprintf(f,"  := 2^-(REF-%i)",zt_var.exponentAxisRange);

	fprintf(f,"\naxisrange = -%.20lg .. +%.20lg\n",-range,+range);

	fprintf(f,"\n\nnotation: e.g. 3.21 = 24\n");
	fprintf(f,"  3 bits for the integer part\n");
	fprintf(f," 21 bits for the fractional part\n");
	fprintf(f," 24 total bits\n\n");

	fprintf(f,"\n===============================================================\n|real z_new| <=\n");
	zt_real.termTab(f);
		
	fprintf(f,"\n===============================================================\n|imag z_new| <=\n");
	zt_imag.termTab(f);

	fclose(f);
}

void formel_z4azc(void) {
	zt_real.setTerme("d + f* x - g *y + x^4 - 6 *x^2 *y^2 + y^4");
	zt_imag.setTerme("e + f* y + g* x + 4* x^3 *y - 4* x* y^3");
	
	zt_var.clearVar();
	zt_var.setValueByChar('d',2);
	zt_var.setValueByChar('e',2);
	zt_var.setValueByChar('f',2);
	zt_var.setValueByChar('g',2);
	
	if (_R>0) {
		formel_z4azc(
			(int32_t)ceil(log(2.0*_R) / log(2.0)),
			_R
		);
	} else {
		for(int32_t bb=1;bb<=3;bb++) {
			formel_z4azc(
				bb+1,
				(1 << bb)
			);
		}
	}
}

void formel_z5azc(const int bits,const double range) {
	zt_var.exponentAxisRange=bits;
		
	zt_var.setValueByChar('x',range);
	zt_var.setValueByChar('y',range);
		
	char tmp[1024];
	sprintf(tmp,"_zertifikat.z5azc_bit.precision_R%.20lg.txt",range);
	FILE *f=fopen(tmp,"wt");
	fprintf(f,"z^5+A*z+c == (x+i*y)^5 + (f+g*i)*(x+i*y) + (d+e*i)\n\n");
	
	fprintf(f,"|re(C)=d|<=%.20lg,|im(C)=e|<=%.20lg\n",
		zt_var.getValueByChar('d'),
		zt_var.getValueByChar('e')
	);
	fprintf(f,"|re(A)=f|<=%.20lg,|im(A)=g|<=%.20lg\n",
		zt_var.getValueByChar('f'),
		zt_var.getValueByChar('g')
	);
	fprintf(f,"|x|<=%.20lg,|y|<=%.20lg\n",
		zt_var.getValueByChar('x'),
		zt_var.getValueByChar('y')
	);

	fprintf(f,"SCALE := (2*axis range) / (2^REFINEMENT LEVEL == pixels per axis)\n");
	fprintf(f,"  := 2^-(REF-%i)",zt_var.exponentAxisRange);

	fprintf(f,"\naxisrange = -%.20lg .. +%.20lg\n",-range,+range);

	fprintf(f,"\n\nnotation: e.g. 3.21 = 24\n");
	fprintf(f,"  3 bits for the integer part\n");
	fprintf(f," 21 bits for the fractional part\n");

	fprintf(f," 24 total bits\n\n");

	fprintf(f,"\n===============================================================\n|real z_new| <=\n");
	zt_real.termTab(f);
		
	fprintf(f,"\n===============================================================\n|imag z_new| <=\n");
	zt_imag.termTab(f);

	fclose(f);
}

void formel_z6azc(const int bits,const double range) {
	zt_var.exponentAxisRange=bits;
		
	zt_var.setValueByChar('x',range);
	zt_var.setValueByChar('y',range);
		
	char tmp[1024];
	sprintf(tmp,"_zertifikat.z6azc_bit.precision_R%.20lg.txt",range);
	FILE *f=fopen(tmp,"wt");
	fprintf(f,"z^6+A*z+c == (x+i*y)^6 + (f+g*i)*(x+i*y) + (d+e*i)\n\n");
	
	fprintf(f,"|re(C)=d|<=%.20lg,|im(C)=e|<=%.20lg\n",
		zt_var.getValueByChar('d'),
		zt_var.getValueByChar('e')
	);
	fprintf(f,"|re(A)=f|<=%.20lg,|im(A)=g|<=%.20lg\n",
		zt_var.getValueByChar('f'),
		zt_var.getValueByChar('g')
	);
	fprintf(f,"|x|<=%.20lg,|y|<=%.20lg\n",
		zt_var.getValueByChar('x'),
		zt_var.getValueByChar('y')
	);

	fprintf(f,"SCALE := (2*axis range) / (2^REFINEMENT LEVEL == pixels per axis)\n");
	fprintf(f,"  := 2^-(REF-%i)",zt_var.exponentAxisRange);

	fprintf(f,"\naxisrange = -%.20lg .. +%.20lg\n",-range,+range);
		
	fprintf(f,"\n\nnotation: e.g. 3.21 = 24\n");
	fprintf(f,"  3 bits for the integer part\n");
	fprintf(f," 21 bits for the fractional part\n");
	fprintf(f," 24 total bits\n\n");

	fprintf(f,"\n===============================================================\n|real z_new| <=\n");
	zt_real.termTab(f);
		
	fprintf(f,"\n===============================================================\n|imag z_new| <=\n");
	zt_imag.termTab(f);

	fclose(f);
}

void formel_z6azc(void) {
	zt_real.setTerme("d+f*x-g*y+x^6-3*5*x^4*y^2+3*5*x^2*y^4-y^6");
	zt_imag.setTerme("f*y+g*x+6*x^5*y-4*5*x^3*y^3+6*x*y^5+e");
	
	zt_var.clearVar();
	zt_var.setValueByChar('d',2);
	zt_var.setValueByChar('e',2);
	zt_var.setValueByChar('f',2);
	zt_var.setValueByChar('g',2);
	
	if (_R>0) {
		formel_z6azc(
			(int32_t)ceil(log(2.0*_R) / log(2.0)),
			_R
		);
	} else {
		for(int32_t bb=1;bb<=3;bb++) {
			formel_z6azc(
				bb+1,
				(1 << bb)
			);
		}
	}
}

void formel_z5azc(void) {
	zt_real.setTerme("d + f*x - g*y + x^5 - 2*5*x^3*y^2 + 5*x*y^4");
	zt_imag.setTerme("e + f*y + g*x + 5*x^4*y - 2*5*x^2*y^3 + y^5");
	
	zt_var.clearVar();
	zt_var.setValueByChar('d',2);
	zt_var.setValueByChar('e',2);
	zt_var.setValueByChar('f',2);
	zt_var.setValueByChar('g',2);
	
	if (_R>0) {
		formel_z5azc(
			(int32_t)ceil(log(2.0*_R) / log(2.0)),
			_R
		);
	} else {
		for(int32_t bb=1;bb<=3;bb++) {
			formel_z5azc(
				bb+1,
				(1 << bb)
			);
		}
	}
}

void formel_tricz4b(const int bits,const double range) {
	zt_var.exponentAxisRange=bits;
		
	zt_var.setValueByChar('x',range);
	zt_var.setValueByChar('y',range);
	zt_var.setValueByChar('z',range);
		
	char tmp[1024];
	sprintf(tmp,"_zertifikat.tricz4b_bit.precision_R%.20lg.txt",range);
	FILE *f=fopen(tmp,"wt");
	fprintf(f,"triplex formula TRICZ4B\n\nx_new := %s\ny_new := %s\nz_new := %s\n\n",
		zt_real.termstr,
		zt_imag.termstr,
		zt_pz.termstr);
	
	fprintf(f,"|f|<=%.20lg,|h|<=%.20lg\n",
		zt_var.getValueByChar('f'),
		zt_var.getValueByChar('h')
	);
	fprintf(f,"|x|<=%.20lg,|y|<=%.20lg,|z|<=%.20lg\n",
		zt_var.getValueByChar('x'),
		zt_var.getValueByChar('y'),
		zt_var.getValueByChar('z')
	);
	fprintf(f,"SCALE := (2*axis range) / (2^REFINEMENT LEVEL == voxels per axis)\n");
	fprintf(f,"  := 2^-(REF-%i)",zt_var.exponentAxisRange);

	fprintf(f,"\naxisrange = -%.20lg .. +%.20lg\n",-range,+range);

	fprintf(f,"\n\nnotation: e.g. 3.21 = 24\n");
	fprintf(f,"  3 bits for the integer part\n");
	fprintf(f," 21 bits for the fractional part\n");
	fprintf(f," 24 total bits\n\n");


	fprintf(f,"\n===============================================================\n|x_new| <=\n");
	zt_real.termTab(f);
		
	fprintf(f,"\n===============================================================\n|y_new| <=\n");
	zt_imag.termTab(f);

	fprintf(f,"\n===============================================================\n|zy_new| <=\n");
	zt_pz.termTab(f);

	fclose(f);
}

void formel_tricz4b(void) {
	zt_real.setTerme("r + f*x+x^4-6*x^2*y^2+y^4-h*z^2");
	zt_imag.setTerme("s + f*y+4*x^3*y-4*y^3*x");
	zt_pz.setTerme("t + z^3-y^3-x^3");
	
	zt_var.clearVar();
	zt_var.setValueByChar('r',2);
	zt_var.setValueByChar('s',2);
	zt_var.setValueByChar('t',2);
	zt_var.setValueByChar('f',2);
	zt_var.setValueByChar('h',2);
	
	if (_R>0) {
		formel_tricz4b(
			(int32_t)ceil(log(2.0*_R) / log(2.0)),
			_R
		);
	} else {
		for(int32_t bb=1;bb<=3;bb++) {
			formel_tricz4b(
				bb+1,
				(1 << bb)
			);
		}
	}
}


// zertifikatTerme

ZertifikatTerme::ZertifikatTerme() {
	anz=0;
	termstr=NULL;
}

ZertifikatTerme::~ZertifikatTerme() {
	if (termstr) delete[] termstr;
}

void ZertifikatTerme::clearAll(void) {
	anz=0;
	if (termstr) delete[] termstr;
	termstr=NULL;
}

void ZertifikatTerme::setTerme(const char* as) {
	clearAll();

	termstr=new char[strlen(as)];
	strcpy(termstr,as);

	char* w=new char[MAXTEXT];
	int wlen=0;
	for(uint32_t i=0;i<strlen(as);i++) {
		if (as[i] != ' ') {
			if ((as[i]=='-') || (as[i]=='+')) w[wlen]=':';
			else w[wlen]=lochar(as[i]);
			wlen++;
			w[wlen]=0;
		}
	}
	sprintf(&w[strlen(w)],":_");
	wlen=strlen(w);
	
	char* pl=w;
	while (1) {
		char* p=strchr(w,':');
		if (!p) break;
		// zw. pl und p-1
		p[0]=0;
		addTerm(pl);
		
		p[0]='_'; // fertig
		pl=p+1;
	} // while
	
	delete[] w;
}

void ZertifikatTerme::addTerm(const char* as) {
	char w[1024];
	int wlen=0;
	w[0]=0;
	for(uint32_t i=0;i<strlen(as);i++) {
		if (as[i] != ' ') {
			w[wlen]=lochar(as[i]);
			wlen++;
			w[wlen]=0;
		}
	}
	pterme[anz]=minimalparseBitPrec_stripped(w);
	anz++;
}

void ZertifikatTerme::termTab(FILE* f) {
	double summe=0.0;
	char *tmp=new char[MAXTEXT];
	
	// exponent of RANGE
	// as it it subtracted later on, rather take too small a value
	// than too big (hence floor)

	char *maxstr=new char[8*MAXTEXT];
	maxstr[0]=0;

	char* t=new char[MAXTEXT];
	char* t2=new char[MAXTEXT];
	for(int32_t i=0;i<anz;i++) {
		double maxwert=ceil(pterme[i]->evalMax());
		summe += maxwert;
		pterme[i]->getText(t2);
		if (i>0) sprintf(t,"+ |%s|",t2);
		else sprintf(t,"|%s|",t2);
		fprintf(f,"\t%20s\t||maxw=%3.0lf\t%i.0\t%s\n",
			t,
			maxwert,
			bitsDec(maxwert),
			pterme[i]->textFrac(tmp)
		);
		if (!strstr(maxstr,tmp)) {
			if (maxstr[0]>0) {
				sprintf(&maxstr[strlen(maxstr)],",%s",tmp);
			} else {
				sprintf(&maxstr[strlen(maxstr)],"%s",tmp);
			}
		}
	}
	
	delete[] t;
	delete[] t2;
	
	fprintf(f,"-----------------------------------------------------------\n");
	int bitsdec=bitsDec(summe);
	fprintf(f,"\t\t                ||maxw=%3.0lf\t%i.0\tmax(%s)\n",
		summe,
		bitsdec,
		maxstr
	);
	
	delete[] tmp;
	delete[] maxstr;
	
	fprintf(f,"\n");

	for(int32_t REF=10;REF<=24;REF++) {
		int32_t bd,bf,bsum;
		
		bd=bitsdec;
		bf=0;
		zt_var.refinement=REF;
		for(int32_t i=0;i<anz;i++) {
			int32_t b=pterme[i]->fracBits();
			if (b > bf) bf=b;
		}
		
		bsum=bd+bf;
		
		fprintf(f,"\t\t\tREF %i\t\t\t%i.%i = %i\t=> ",
			REF,bd,bf,bsum);
		
		if (bsum < 53) fprintf(f,"double\n");	
		else if (bsum < 63) fprintf(f,"long double\n");	
		else if (bsum < 113) fprintf(f,"float128\n");	
		else fprintf(f,"arbitrary\n");
	}
}


// zertifikatVar

int ZertifikatVar::getFracBitsByIdx(const int idx) {
	if ( (idx < 0) || (idx > 25) ) return 256;
	
	return fracbits[idx];
}

ZertifikatVar::ZertifikatVar() {
	clearVar();
}

void ZertifikatVar::clearVar(void) {
	for(int32_t i=0;i<=25;i++) {
		maxw[i]=0;
		fracbits[i]=BASEDENOM;
		used[i]=0;
	}
	exponentAxisRange=0;
}

void ZertifikatVar::setFracBitsByIdx(const int idx,const int fb) {
	fracbits[idx]=fb;
	used[idx]=1;
}

void ZertifikatVar::setValueByIdx(const int idx,const double aw) {
	maxw[idx]=aw;
	used[idx]=1;
}

void ZertifikatVar::setValueByChar(const char ach,const double aw) {
	int32_t idx=getVarIdx(ach);
	if ((idx < 0)||(idx >= 26)) {
		printf("Fehler. setVar Falsche Variable %c (#%i)\n",ach,idx);
		exit(99);
	}
	maxw[idx]=aw;
	used[idx]=1;
}

double ZertifikatVar::getValueByChar(const char ch) {
	return getValueByIdx(getVarIdx(ch));
}

double ZertifikatVar::getValueByIdx(const int idx) {
	if ((idx < 0)||(idx >= 26)) {
		printf("getVar. Fehler. Falsche Variable %i\n",idx);
		exit(99);
	}

	return maxw[idx];
}

int getfunc(const char* s) {
	char tmp[2048];
	strcpy(tmp,s); upper(tmp);
	for(int32_t i=0;i<FUNCANZ;i++) {
		if (!strcmp(tmp,funcname[i])) return i;
	}
	
	return -1;
}

int main(int argc,char** argv) {
	FUNC=FUNC_Z2C;
	_R=0;
	
	for(int32_t i=1;i<argc;i++) {
		upper(argv[i]);
		if (strstr(argv[i],"FUNC=")==argv[i]) {
			FUNC=getfunc(&argv[i][5]);
			if (FUNC<0) {
				FUNC=FUNC_USER;
				int l=strlen(argv[i]);
				if (l>1000) argv[i][1000]=0;
				strcpy(funcuserstr,&argv[i][5]);
				lower(funcuserstr);
			}
		} else 
		if (strstr(argv[i],"RANGE=")==argv[i]) {
			int32_t a;
			if (sscanf(&argv[i][6],"%i",&a) == 1) {
				_R=a;
			}
		} else {
			// is it a one-letter real variable
			// char,max abs value,fractional bit precision
			char ch;
			int32_t maxabs,fracb;
			if (sscanf(argv[i],"%c=%i,%i",&ch,&maxabs,&fracb) == 3) {
				int32_t idx=getVarIdx(ch);
				if (maxabs<0) maxabs=-maxabs;
				if ((idx>=0) && (idx<=25)) {
					zt_var.setValueByIdx(idx,maxabs);
					zt_var.setFracBitsByIdx(idx,fracb);;
				}
			}
		}
	}
	
	switch (FUNC) {
		case FUNC_Z2C: formel_z2c(); break;
		case FUNC_Z3AZC: formel_z3azc(); break;
		case FUNC_Z4AZC: formel_z4azc(); break;
		case FUNC_Z5AZC: formel_z5azc(); break;
		case FUNC_Z6AZC: formel_z6azc(); break;
		case FUNC_TRICZ4B: formel_tricz4b(); break;
		case FUNC_USER: formel_user(funcuserstr); break;
		default: {
			printf("Error. Undefined function.\n");
			exit(99);
			break;
		}
	}

	return 0;
}
