
#ifndef DATASAFE_H
#define	DATASAFE_H
#include <stddef.h>
#include <time.h>
#pragma hdrstop
#define	S_LEN	8			/*   单密钥长度  */
#define	D_LEN	16			/*   双密钥长度  */
#define	PIN_LEN	8			/*   PIN 长度    */
#define DES_KEY "@WSX$RFV"

//---------------------------------------------------------------------------
typedef unsigned char uchar;
typedef int bool;
#define false 0
#define true 1
unsigned char C[17][28],D[17][28],K[17][48],c,ch;


void  DES(uchar *key,uchar *text,uchar *mtext)
{
	uchar tmp[64];
	expand0(key,tmp);
	setkeystar(tmp);
	encrypt0(text,mtext);
}
void _DES(uchar *key,uchar *text,uchar *mtext)
{
	uchar tmp[64];
	expand0(key,tmp);
	setkeystar(tmp);
	discrypt0(text,mtext);
}
void encrypt0(uchar *text,uchar *mtext)
{
	uchar ll[64],rr[64],LL[64],RR[64];
	uchar tmp[64];
	int i,j;
	ip(text,ll,rr);

	for (i=1;i<17;i++)
	{
		F(i,ll,rr,LL,RR);
		for (j=0;j<32;j++)
		{
			ll[j]=LL[j];
			rr[j]=RR[j];
		}
	}

	_ip(tmp,rr,ll);

	compress0(tmp,mtext);
}

void discrypt0(uchar *mtext,uchar *text)
{
	uchar ll[64],rr[64],LL[64],RR[64];
	uchar tmp[64];
	int i,j;
	ip(mtext,ll,rr);

	for (i=16;i>0;i--)
	{
		F(i,ll,rr,LL,RR);
		for (j=0;j<32;j++)
		{
			ll[j]=LL[j];
			rr[j]=RR[j];
		}
	}

	_ip(tmp,rr,ll);

	compress0(tmp,text);
}


void expand0(uchar *in,uchar *out)         //取得64位bit=1的顺序号
{
	int divide;
	int i,j;

	for (i=0;i<8;i++)
	{
		divide=0x80;
		for (j=0;j<8;j++)
		{
			*out++=((in[i]/divide)&1);
			divide/=2;
		}
	}
}


void compress0(uchar *out,uchar *in)
{
	int times;
	int i,j;

	for (i=0;i<8;i++)
	{
		times=0x80;
		in[i]=0;
		for (j=0;j<8;j++)
		{
			in[i]+=(*out++)*times;
			times/=2;
		}
	}
}
void compress016(uchar *out,uchar *in)
{
	int times;
	int i,j;

	for (i=0;i<16;i++)
	{
		times=0x8;
		in[i]='0';
		for (j=0;j<4;j++)
		{
			in[i]+=(*out++)*times;
			times/=2;
		}
	}
}

int pc_1_c[28]={
	57,49,41,33,25,17,9
	,1,58,50,42,34,26,18
	,10,2,59,51,43,35,27
	,19,11,3,60,52,44,36};
int pc_1_d[28]={
	63,55,47,39,31,23,15
	,7,62,54,46,38,30,22
	,14,6,61,53,45,37,29
	,21,13,5,28,20,12,4};
int pc_2[48]={
	14,17,11,24,1,5,
	3,28,15,6,21,10,
	23,19,12,4,26,8,
	16,7,27,20,13,2,
	41,52,31,37,47,55,
	30,40,51,45,33,48,
	44,49,39,56,34,53,
	46,42,50,36,29,32};

int ls_count[16]={
	1,1,2,2,2,2,2,2,1,2,2,2,2,2,2,1};

void setkeystar(uchar bits[])
{
	int i,j;

	for (i=0;i<28;i++)
		C[0][i]=bits[pc_1_c[i]-1];
	for (i=0;i<28;i++)
		D[0][i]=bits[pc_1_d[i]-1];
	for (j=0;j<16;j++)
	{
		LS(C[j],C[j+1],ls_count[j]);
		LS(D[j],D[j+1],ls_count[j]);
		son(C[j+1],D[j+1],K[j+1]);
	}
}


void LS(uchar *bits,uchar *buffer,int count)
{
	int i,j;
	for (i=0;i<28;i++)
	{
		buffer[i]=bits[(i+count)%28];
	}
}

void son(uchar *cc,uchar *dd,uchar *kk)
{
	int i,j;
	uchar buffer[56];

	for (i=0;i<28;i++)
		buffer[i]=*cc++;

	for (i=28;i<56;i++)
		buffer[i]=*dd++;

	for (i=0;i<48;i++)
		*kk++=buffer[pc_2[i]-1];
}
int ip_tab[64]={
	58,50,42,34,26,18,10,2,
	60,52,44,36,28,20,12,4,
	62,54,46,38,30,22,14,6,
	64,56,48,40,32,24,16,8,
	57,49,41,33,25,17,9,1,
	59,51,43,35,27,19,11,3,
	61,53,45,37,29,21,13,5,
	63,55,47,39,31,23,15,7};

int _ip_tab[64]={
	40,8,48,16,56,24,64,32,
	39,7,47,15,55,23,63,31,
	38,6,46,14,54,22,62,30,
	37,5,45,13,53,21,61,29,
	36,4,44,12,52,20,60,28,
	35,3,43,11,51,19,59,27,
	34,2,42,10,50,18,58,26,
	33,1,41,9,49,17,57,25};

void ip(uchar *text,uchar *ll,uchar *rr)
{
	int i,j;
	uchar buffer[64];
	uchar tmp[64];
	expand0(text,buffer);

	for (i=0;i<32;i++)
		ll[i]=buffer[ip_tab[i]-1];

	for (i=0;i<32;i++)
		rr[i]=buffer[ip_tab[i+32]-1];
}

void _ip(uchar *text,uchar *ll,uchar *rr)
{
	int i,j;
	uchar tmp[64];
	for (i=0;i<32;i++)
		tmp[i]=ll[i];
	for (i=32;i<64;i++)
		tmp[i]=rr[i-32];
	for (i=0;i<64;i++)
		text[i]=tmp[_ip_tab[i]-1];
}


int e_r[48]={
	32,1,2,3,4,5,4,5,6,7,8,9,
	8,9,10,11,12,13,12,13,14,15,16,17,
	16,17,18,19,20,21,20,21,22,23,24,25,
	24,25,26,27,28,29,28,29,30,31,32,1};

int P[32]={
	16,7,20,21,29,12,28,17,
	1,15,23,26,5,18,31,10,
	2,8,24,14,32,27,3,9,
	19,13,30,6,22,11,4,25};
int SSS[16][4][16]={
	14,4,13,1,2,15,11,8,3,10,6,12,5,9,0,7,
	0,15,7,4,14,2,13,1,10,6,12,11,9,5,3,8,/* err on */
	4,1,14,8,13,6,2,11,15,12,9,7,3,10,5,0,
	15,12,8,2,4,9,1,7,5,11,3,14,10,0,6,13,

	15,1,8,14,6,11,3,4,9,7,2,13,12,0,5,10,
	3,13,4,7,15,2,8,14,12,0,1,10,6,9,11,5,
	0,14,7,11,10,4,13,1,5,8,12,6,9,3,2,15,
	13,8,10,1,3,15,4,2,11,6,7,12,0,5,14,9,

	10,0,9,14,6,3,15,5,1,13,12,7,11,4,2,8,
	13,7,0,9,3,4,6,10,2,8,5,14,12,11,15,1,
	13,6,4,9,8,15,3,0,11,1,2,12,5,10,14,7,
	1,10,13,0,6,9,8,7,4,15,14,3,11,5,2,12,

	7,13,14,3,0,6,9,10,1,2,8,5,11,12,4,15,
	13,8,11,5,6,15,0,3,4,7,2,12,1,10,14,9,
	10,6,9,0,12,11,7,13,15,1,3,14,5,2,8,4,
	3,15,0,6,10,1,13,8,9,4,5,11,12,7,2,14, /* err on */

	2,12,4,1,7,10,11,6,8,5,3,15,13,0,14,9,
	14,11,2,12,4,7,13,1,5,0,15,10,3,9,8,6, /* err on */
	4,2,1,11,10,13,7,8,15,9,12,5,6,3,0,14,
	11,8,12,7,1,14,2,13,6,15,0,9,10,4,5,3,

	12,1,10,15,9,2,6,8,0,13,3,4,14,7,5,11,
	10,15,4,2,7,12,9,5,6,1,13,14,0,11,3,8,
	9,14,15,5,2,8,12,3,7,0,4,10,1,13,11,6,
	4,3,2,12,9,5,15,10,11,14,1,7,6,0,8,13,

	4,11,2,14,15,0,8,13,3,12,9,7,5,10,6,1,
	13,0,11,7,4,9,1,10,14,3,5,12,2,15,8,6,
	1,4,11,13,12,3,7,14,10,15,6,8,0,5,9,2,
	6,11,13,8,1,4,10,7,9,5,0,15,14,2,3,12,

	13,2,8,4,6,15,11,1,10,9,3,14,5,0,12,7,
	1,15,13,8,10,3,7,4,12,5,6,11,0,14,9,2,
	7,11,4,1,9,12,14,2,0,6,10,13,15,3,5,8,
	2,1,14,7,4,10,8,13,15,12,9,0,3,5,6,11};

void F(int n,uchar *ll,uchar *rr,uchar *LL,uchar *RR)
{
	int i,j;
	uchar buffer[64],tmp[64];
	for (i=0;i<48;i++)
		buffer[i]=rr[e_r[i]-1];
	for (i=0;i<48;i++)
		buffer[i]=(buffer[i]+K[n][i])&1;

	s_box(buffer,tmp);

	for (i=0;i<32;i++)
		buffer[i]=tmp[P[i]-1];

	for (i=0;i<32;i++)
		RR[i]=(buffer[i]+ll[i])&1;

	for (i=0;i<32;i++)
		LL[i]=rr[i];


}

void s_box(uchar *aa,uchar *bb)
//uchar *aa,*bb;
{
	int i,j,k,m;
	int y,z;
	uchar ss[8];
	m=0;
	for (i=0;i<8;i++)
	{
		j=6*i;
		y=aa[j]*2+aa[j+5];
		z=aa[j+1]*8+aa[j+2]*4+aa[j+3]*2+aa[j+4];
		ss[i]=SSS[i][y][z];
		y=0x08;
		for (k=0;k<4;k++)
		{
			bb[m++]=(ss[i]/y)&1;
			y/=2;
		}

	}
}

//--------------------------------------------------------------------
//--------------------------------------------------------------------

/*---------------------
Single & Double DES
---------------------*/
int	sssDES (
	uchar key[],
	char key_length,
	uchar in[],
	char in_length,
	uchar out[]
	)
{
	if ( key_length == 8 ) {	/* single key, single input */
                if(in_length==16){      /* single key, double input */
                     DES ( key, in, out );
                     _DES( key, out, out );
          	     DES ( key, out, out );
      	             DES ( key, in+S_LEN, out+S_LEN );
	             _DES ( key, out+S_LEN, out+S_LEN );
	             DES ( key, out+S_LEN, out+S_LEN );
            	     return ( 0 );
                }
		if ( in_length != 8 )
			return ( -1 );
		DES ( key, in, out );
		return ( 0 );
	}

	if ( key_length != 16 )
		return ( -1);

	if ( in_length == 8 ) {	    /* double key, single input */
		DES ( key, in, out );
		_DES ( key+S_LEN, out, out );
		DES ( key, out, out );
		return ( 0 );
	}

	if ( in_length != 16 )
		return ( -1 );

	DES ( key, in, out );		/* double key, double input */
	_DES ( key+S_LEN, out, out );
	DES ( key, out, out );
	DES ( key, in+S_LEN, out+S_LEN );
	_DES ( key+S_LEN, out+S_LEN, out+S_LEN );
	DES ( key, out+S_LEN, out+S_LEN );
	return ( 0 );
}


/*---------------------
Single & Double _DES
---------------------*/
int	_sssDES (
	uchar key[],
	char key_length,
	uchar in[],
	char in_length,
	uchar out[]
	)
{
	if ( key_length == 8 ) {	/* single key, single input */
     	        if ( in_length == 16 ){         /* single key, double input */
                _DES ( key, in, out );
                DES  ( key, out, out );
        	_DES ( key, out, out );
	        _DES ( key, in+S_LEN, out+S_LEN );
        	DES  ( key, out+S_LEN, out+S_LEN );
	        _DES ( key, out+S_LEN, out+S_LEN );
        	return ( 0 );
                }

		if ( in_length != 8 )
			return ( -1 );
		_DES ( key, in, out );
		return ( 0 );
	}

	if ( key_length != 16 )
		return ( -1 );

	if ( in_length == 8 ) {	/* double key, single input */
		_DES ( key, in, out );
		DES ( key+S_LEN, out, out );
		_DES ( key, out, out );
		return ( 0 );
	}

	if ( in_length != 16 )
		return ( -1 );

	_DES ( key, in, out );		/* double key, double input */
	DES ( key+S_LEN, out, out );
	_DES ( key, out, out );
	_DES ( key, in+S_LEN, out+S_LEN );
	DES ( key+S_LEN, out+S_LEN, out+S_LEN );
	_DES ( key, out+S_LEN, out+S_LEN );
	return ( 0 );
}


/*----------------------------------------------------
Generate key from components
----------------------------------------------------*/
uchar	component_para [512]={
	0x12, 0x38, 0xab, 0x67, 0xcb, 0x91, 0x54, 0x9a,
	0x00, 0x12, 0xfb, 0x00, 0xcb, 0xfb, 0x38, 0x9a,
	0x44, 0x38, 0x12, 0x67, 0x00, 0x38, 0xfb, 0x00,
	0xfb, 0xd0, 0x38, 0x12, 0x9a, 0x2a, 0x54, 0x54,
	0xfd, 0x2a, 0x00, 0x54, 0x12, 0xcb, 0x9a, 0x9a,
	0x2a, 0xd0, 0xab, 0x00, 0x00, 0x12, 0x2a, 0x99,
	0x44, 0x99, 0xd0, 0x67, 0x91, 0x99, 0x12, 0x9b,
	0x9a, 0x44, 0x9b, 0xab, 0x00, 0x91, 0x00, 0x12,
	};

int	components_key (
	int component_number,
	uchar *components,
	int key_no,
	uchar *key
	)
{
int	i,	j;
uchar	c,	tmp [ D_LEN ];

	for ( i=0; i<D_LEN; i++ ) {
		c = 0x00;
		for ( j=0; j<component_number; j++ )
			c ^= components [ j * D_LEN + i ];
		tmp [ i ] = c ^ component_para [ i + key_no ] ;
	}
	memcpy ( key, tmp, D_LEN );
	return ( 0 );
}


/*-------------------------
Generate a Random Number
-------------------------*/
int	sss_rn_gen (
	char rn_length,	/* input */
	uchar rn[]		/* output */
	)
{
int	i,	j;
static	uchar seed = 0;
long	clock;
uchar	* ptr;
unsigned	* ui;

	if ( ! seed ) {
		//time (&(clock) );
		//time_t t;
		//time(&t);
		//CTime ctime(t);	
		//clock = ctime.GetTime();
                                clock = time(NULL); 
		ptr = ( uchar * )&clock + sizeof( long ) - sizeof( unsigned );
		ui = ( unsigned * ) ptr;
		srand ( (*ui) );
		seed = 1;
	}

	if ( rn_length == 8)
		j = 8;
	else	if ( rn_length == 16 )
			j = 16;
		else	return ( -1);

	for ( i=0; i<j; i++ )
		rn [ i ] = ( uchar ) ( rand () % 256 );

	return ( 0 );
}


/*----------------------------
Calculate key offset
----------------------------*/
int	offset_key (			/* input */
	uchar key[],			/* key */
	uchar counter[],		/* counter */
				/* output */
	uchar key_offset[]		/* key offset */
	)
{
int	i,	j;
uchar	c,	tmp [ D_LEN ];
uchar 	counter1[D_LEN];

	memcpy ( counter1 , counter , 7 );
	memcpy ( counter1+7 , counter , 7 );

	memcpy ( tmp, key, D_LEN );
	for ( i=0; i<D_LEN; i++ ) {
		for ( j=0; j<7; j++ ) {
		c = (( counter1[ (i*7+j)/8 ] << ((i*7+j)%8) ) & 0x80) >> j;
		tmp [ i ] = tmp [ i ] ^ c;
		}
	}
	memcpy ( key_offset, tmp, D_LEN );
	return ( 0 );
}

/*--------------------------------------
entry:	asc
	len	--- the length of asc
return:	0 --- SUCC
	-1 --- FAIL
	bcd
----------------------------------------*/
int asctobcd(char *bcd, char *asc, int len)
{
char	*p;
int	i, j;

	p = (char *) malloc( (len+1) * sizeof(char) );

	for(i = 0; i < len; i++) {
		if( (asc[i] >= '0') && (asc[i] <= '9') )
			p[i] = asc[i] - '0';
		else if( (asc[i] >= 'a') && (asc[i] <= 'f') )
			p[i] = asc[i] - 'a' + 10;
		else if( (asc[i] >= 'A') && (asc[i] <= 'F') )
			p[i] = asc[i] - 'A' + 10;

		else {
			free(p);
//			errcall(ERROR,"error word!!!!\n");
			return(-1);
		}
	}

	j = (len + len%2) / 2;
	if( len%2 ) p[len+1] = 0;

	for (i=0; i<j; i++) {
		bcd[i] = (p[2*i] & 0x0f)<<4;
		bcd[i] |= (p[2*i+1] & 0x0f);
	}

	free(p);
	return(0);
}

/*-------------------------------------------
entry:	bcd
	len	---  the length of asc
return:	asc
---------------------------------------------*/
int bcdtoasc(char *asc, char *bcd, int len)
{
int  i, j;
	j = (len + len%2) / 2;
	for(i=0; i<j; i++) {
		asc[2*i] = (bcd[i] >> 4) & 0x0f;
		asc[2*i+1] = bcd[i] & 0x0f;
	}
	for(i=0; i<len; i++) {
		if( asc[i] > 0x09)
			asc[i] = 'A' + asc[i] - 0x0a;
		else	asc[i] += '0';
	}

	asc[len] = 0;
	return ( 0 );
}

/*
加密
密码长度8或16
文本长度8或16
*/
void singleDES(char *key,int key_length,char *in,int in_length,char *out)
{
    char bcdtxt[50];
    memset(bcdtxt,0,sizeof(bcdtxt));
    if(key_length!=8 && key_length!=16) return;
    if(in_length!=8 && in_length!=16) return;
    sssDES((uchar *)key,key_length,(uchar *)in,in_length,(uchar *)bcdtxt);
    bcdtoasc(out,bcdtxt,in_length*2);
}

/*
解密密
密码长度8或16
文本长度8或16
*/
void _singleDES(char *key,int key_length,char *in,int in_length,char *out)
{
    char bcdtxt[50];
    memset(bcdtxt,0,sizeof(bcdtxt));
    if(key_length!=8 && key_length!=16) return;
    if(in_length!=8 && in_length!=16) return;
    asctobcd(bcdtxt,in,in_length*2);
    _sssDES((uchar *)key,key_length,(uchar *)bcdtxt,in_length,(uchar*)out);
}

char IntToHex(int len,bool lowcase)
{

     char ch=0;
     if(len<10 && len>=0)
     {
        ch = 48+len;
     }else
     if(len>=10 && len<16)
     {
        if(lowcase){
           ch = 90+len;
           return ch;
        }
        ch = 55+len;
      }
      return ch;
}

int HexToInt(char ch)
{
   int num=0;
      if(ch<=57 && ch>=48)
      {
          num = ch-48;
      }else
      if(ch>=65 && ch<=70)
      {
          num = ch-55;
      }else
      if(ch>=100 && ch<=105)
      {
          num = ch-90;
      }
      return num;
}

/*
   密码8位
   文本小于等于16位
*/

void randomDES(char *key,char *in,int in_length,char *out)
{
    char randomKey[10];
    char randombcdtxt[60];
    char randomTxt[60];
    char intxt[20];

    int inlen=in_length;
    memset(randomTxt,0,sizeof(randomTxt));
    memset(intxt,0,sizeof(intxt));
    if(in_length==0) return;

    sss_rn_gen(8,(uchar *)randomTxt);
    if(in_length>=16){
       inlen=0;
       memcpy(intxt,in,16);
    }else{
       inlen=in_length;
       memcpy(intxt,in,in_length);
       if(in_length<8){
          memcpy(&intxt[in_length],randomTxt,8-in_length);
       }else
       if(in_length>8 && in_length<16){
          memcpy(&intxt[in_length],randomTxt,16-in_length);
       }
    }
    memset(randomKey,0,sizeof(randomKey));
    memcpy(randomKey,&randomTxt[5],3);
    randomKey[3]=IntToHex(inlen,false);
    memcpy(&randomKey[4],&key[4],4); 

    memset(randombcdtxt,0,sizeof(randombcdtxt));
    singleDES(randomKey,8,intxt,strlen(intxt),randombcdtxt); 

    memset(randomTxt,0,sizeof(randomTxt));
    memcpy(randomTxt,randomKey,4);                              
    memcpy(&randomTxt[4],randombcdtxt,4);
    singleDES(key,8,randomTxt,8,out);
    strcat(out,&randombcdtxt[4]); 
}
void _randomDES(char *key,char *in,int in_length,char *out)
{
    char randomKey[10];
    char randombcdtxt[60];
    char randomTxt[60];
    int lenkey=0;
    if(in_length!=28 && in_length!=44) return;
    memset(randombcdtxt,0,sizeof(randombcdtxt));
    memset(randomTxt,0,sizeof(randomTxt));
    memcpy(randomTxt,in,16);
    _singleDES(key,8,randomTxt,8,randombcdtxt);

    memset(randomKey,0,sizeof(randomKey));
    memcpy(randomKey,randombcdtxt,4);      //前4
    memcpy(&randomKey[4],&key[4],4);       //后4

    memset(randomTxt,0,sizeof(randomTxt));
    memcpy(randomTxt,&randombcdtxt[4],4);   //密文前4
    strcat(randomTxt,&in[16]);              //后续密文

    memset(randombcdtxt,0,sizeof(randombcdtxt));
    _singleDES(randomKey,8,randomTxt,strlen(randomTxt)/2,randombcdtxt);

    memset(randomTxt,0,sizeof(randomTxt));
    lenkey =HexToInt(randomKey[3]);
    if(lenkey>0){
      memcpy(randomTxt,randombcdtxt,lenkey);
      strcpy(out,randomTxt);
    }else{
      strcpy(out,randombcdtxt);
    }

}

void _encrypts(char *in,int in_length,char *out)
{
        int t_len=in_length;
        int r_len=t_len%44;
        int num=t_len/44;
		char dest[45];
		char *op=in;
		int i=0;
		
		if(t_len<44)
		{
        	_randomDES(DES_KEY,in,in_length,out);			
			return;
		}

        for(i=0;i<num;i++){
        	char sour[45];
        	memset(sour,0,sizeof(sour));
        	memset(dest,0,sizeof(dest));
        	op=in+i*44;
        	memcpy(sour,op,44);
        	_randomDES(DES_KEY,sour,strlen(sour),dest);
        	strcat(out,dest);
        }
        if(r_len>0){
        	char sour[45];
        	memset(sour,0,sizeof(sour));
			memset(dest,0,sizeof(dest));
        	op=in+i*44;
        	memcpy(sour,op,r_len);
        	_randomDES(DES_KEY,sour,strlen(sour),dest);
        	strcat(out,dest);
        }
}

void encrypt(char *in,int in_length,char *out)
{
        int t_len=in_length;
        int r_len=t_len%16;
        int num=t_len/16;
		char dest[45];
		char *op=in;
		int i=0;
		if(t_len<16)
		{
        	randomDES(DES_KEY,in,in_length,out);
			return;
		}
        for(i=0;i<num;i++){
        	char sour[45];
        	memset(sour,0,sizeof(sour));
        	memset(dest,0,sizeof(dest));
        	op=in+i*16;
        	memcpy(sour,op,16);
        	randomDES(DES_KEY,sour,strlen(sour),dest);
        	strcat(out,dest);
        }
        if(r_len>0){
        	char sour[45];
        	memset(sour,0,sizeof(sour));
        	memset(dest,0,sizeof(dest));
        	op=in+i*16;
        	memcpy(sour,op,r_len);
        	randomDES(DES_KEY,sour,strlen(sour),dest);
        	strcat(out,dest);
        }

}


#endif	/* DATASAFE_H */

