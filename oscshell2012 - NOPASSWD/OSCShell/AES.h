#ifndef AES_H_
#define AES_H_

#include <string.h>
#include <stdlib.h>
#include <atltime.h>

//#define BYTE  unsigned char
//#define LPBYTE unsigned char*

#define MODE_AES128  0
#define MODE_AES192  1
#define MODE_AES256  2


class AES
{
public:
typedef enum ENUM_KeySize_
{
	BIT128 = 0,
	BIT192,
	BIT256
}ENUM_KEYSIZE;
public:
	AES( ENUM_KEYSIZE keysize, BYTE *key_asc);
	~AES(void);

	void Cipher( BYTE *input_asc, BYTE *output_bcd);
    void Cipher( BYTE *input_asc, int input_len,BYTE *output_asc);

	void InvCipher( BYTE *input_bcd, BYTE *output_asc);
	void InvCipher( BYTE *input_asc, int input_len,BYTE *output_asc);
protected:
	BYTE *RotWord( BYTE *word );
	BYTE *SubWord( BYTE *word );
	void AddRoundKey(int round);
	void SubBytes();
	void InvSubBytes();
	void ShiftRows();
	void InvShiftRows();
	void MixColumns();
	void InvMixColumns();
	 static BYTE gfmultby01(BYTE b)
    {
      return b;
    }

    static BYTE gfmultby02(BYTE b)
    {
      if (b < 0x80)
        return (BYTE)(int)(b <<1);
      else
        return (BYTE)( (int)(b << 1) ^ (int)(0x1b) );
    }

    static BYTE gfmultby03(BYTE b)
    {
      return (BYTE) ( (int)gfmultby02(b) ^ (int)b );
    }

    static BYTE gfmultby09(BYTE b)
    {
      return (BYTE)( (int)gfmultby02(gfmultby02(gfmultby02(b))) ^
                     (int)b );
    }

    static BYTE gfmultby0b(BYTE b)
    {
      return (BYTE)( (int)gfmultby02(gfmultby02(gfmultby02(b))) ^
                     (int)gfmultby02(b) ^
                     (int)b );
    }

    static BYTE gfmultby0d(BYTE b)
    {
      return (BYTE)( (int)gfmultby02(gfmultby02(gfmultby02(b))) ^
                     (int)gfmultby02(gfmultby02(b)) ^
                     (int)(b) );
    }

    static BYTE gfmultby0e(BYTE b)
    {
      return (BYTE)( (int)gfmultby02(gfmultby02(gfmultby02(b))) ^
                     (int)gfmultby02(gfmultby02(b)) ^
                     (int)gfmultby02(b) );
    }
	int Nb;
	int Nk;
	int Nr;
	BYTE *key;// the seed key. size will be 4 * keySize from ctor.
	typedef struct BYTE4_
	{
		BYTE w[4];
	}BYTE4;
	BYTE4 *w;
	LPBYTE State[4];
	/*
    private byte[,] iSbox;  // inverse Substitution box 
    private byte[,] w;      // key schedule array.
    private byte[,] Rcon;   // Round constants.
    private byte[,] State;  // State matrix*/

};

extern void _encrypt(BYTE *input_asc,int len,BYTE *output_asc,int mode_type);
extern void _decrypt(BYTE *input_asc,int len,BYTE *output_asc,int mode_type);
//AES key:128bit,16bytes
extern void AES128_decrypt(BYTE *input_asc,int len,BYTE *output_asc);
extern void AES128_encrypt(BYTE *input_asc,int len,BYTE *output_asc);
//AES key:192bit,24bytes
extern void AES192_decrypt(BYTE *input_asc,int len,BYTE *output_asc);
extern void AES192_encrypt(BYTE *input_asc,int len,BYTE *output_asc);
//AES key:256bit,32bytes
extern void AES256_decrypt(BYTE *input_asc,int len,BYTE *output_asc);
extern void AES256_encrypt(BYTE *input_asc,int len,BYTE *output_asc);

#endif /*AES_H_*/