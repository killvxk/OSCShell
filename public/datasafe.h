//---------------------------------------------------------------------------
#ifndef datasafeH
//#if !defined(datasafeH)
#define datasafeH
#include <Windows.h>
//#include <condefs.h>
#include <fstream>
class DATASAFE
{
private:
    void DES(unsigned char *key,unsigned char *text,unsigned char *mtext);
    void _DES(unsigned char *key,unsigned char *text,unsigned char *mtext);
    void encrypt0(unsigned char *text,unsigned char *mtext);
    void discrypt0(unsigned char *mtext,unsigned char *text);
    void expand0(unsigned char *in,unsigned char *out);         //取得64位bit=1的顺序号
    void compress0(unsigned char *out,unsigned char *in);
    void compress016(unsigned char *out,unsigned char *in);
    void setkeystar(unsigned char bits[]);
    void LS(unsigned char *bits,unsigned char *buffer,int count);
    void son(unsigned char *cc,unsigned char *dd,unsigned char *kk);
    void ip(unsigned char *text,unsigned char *ll,unsigned char *rr);
    void _ip(unsigned char *text,unsigned char *ll,unsigned char *rr);
    void F(int n,unsigned char *ll,unsigned char *rr,unsigned char *LL,unsigned char *RR);
    void s_box(unsigned char *aa,unsigned char *bb);
    int	components_key (int component_number,unsigned char *components,int key_no,unsigned char *key);
    int	offset_key (unsigned char key[],unsigned char counter[],unsigned char key_offset[]);

public:
    char IntToHex(int len,bool lowcase);
    int HexToInt(char ch);

    int	sss_rn_gen (char rn_length,unsigned char rn[]);
    int asctobcd(char *bcd, char *asc, int len);
    int bcdtoasc(char *asc, char *bcd, int len);

    int	sssDES (unsigned char key[],char key_length,unsigned char in[],char in_length,unsigned char out[]);
    int	_sssDES (unsigned char key[],char key_length,unsigned char in[],char in_length,unsigned char out[]);

    void singleDES(char *key,int key_length,char *in,int in_length,char *out);
    void _singleDES(char *key,int key_length,char *in,int in_length,char *out);

    void randomDES(char *key,char *in,int in_length,char *out);
    void _randomDES(char *key,char *in,int in_length,char *out);

    void encrypt(char *in,int in_length,char *out);
    void _encrypt(char *in,int in_length,char *out);

    DATASAFE(void);
    ~DATASAFE(void);
};
extern DATASAFE DataSafe;
#endif
