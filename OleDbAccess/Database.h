
#define CATCHERROR(ptr,a)	catch(_com_error &e)\
							{\
								ErrorHandler(e,m_ErrStr);\
								ptr=NULL;\
								return a;\
							}

#define CATCHERRGET			catch(_com_error &e)\
							{\
								ErrorHandler(e,m_ErrStr);\
								sprintf(m_ErrStr,"%s\n**For Field Name:%s",m_ErrStr,FieldName);\
								return 0;\
							}

//#import "c:\Program Files\Common Files\System\ADO\msado15.dll" \
//              rename("EOF", "EndOfFile")
#import "c:\Program Files\Common Files\System\ADO\msado15.dll" \
	rename("EOF", "EndOfFile"),rename("LockTypeEnum","AdoLockTypeEnum"),rename("DataTypeEnum","AdoDataTypeEnum"),\
	rename("FieldAttributeEnum","AdoFieldAttributeEnum"),rename("EditModeEnum","AdoEditModeEnum"),rename("RecordStatusEnum","AdoRecordStatusEnum"),rename("ParameterDirectionEnum","AdoParameterDirectionEnum")



typedef ADODB::_RecordsetPtr	RecPtr;
typedef ADODB::_ConnectionPtr	CnnPtr; 

class Database;
class Table;


class Database
{
private:
	bool isOpen;
public:
	CnnPtr m_Cnn;
	char m_ErrStr[1024];
	Database();
	~Database();
	bool Open(char* UserName, char* Pwd,char* CnnStr);
	bool OpenTbl(int Mode, char* CmdStr, Table *Tbl);
	bool Execute(char* CmdStr);
	bool Execute(char* CmdStr, Table *Tbl);
	void GetErrorErrStr(char* ErrStr);
};

class Table{
public:
	RecPtr m_Rec;
	char m_ErrStr[500];
	Table();
	~Table();
	void GetErrorErrStr(char* ErrStr);
	int ISEOF();
	HRESULT MoveNext();
	HRESULT MovePrevious();
	HRESULT MoveFirst();
	HRESULT MoveLast();
	int AddNew();
	int Update();
	int Add(char* FieldName, char* FieldValue);
	int Add(char* FieldName,int FieldValue);
	int Add(char* FieldName,float FieldValue);
	int Add(char* FieldName,double FieldValue);
	int Add(char* FieldName,long FieldValue);

	bool Get(char* FieldName, char* FieldValue);	
	bool Get(char* FieldName,int& FieldValue);
	bool Get(char* FieldName,float& FieldValue);
	bool Get(char* FieldName,double& FieldValue);
	bool Get(char* FieldName,double& FieldValue,int Scale);
	bool Get(char* FieldName,long& FieldValue);
};

