#ifndef OLEDBACCESS_H
#define OLEDBACCESS_H
#include "TransBusi.h"

class COleDbAccess{
private:
	TRANSBUSI *busiHandle;
public:
	void beginTrans(void);
	void commitTrans(void);
	void rollbackTrans(void);
	bool oleDb_Open(char *dbtype,char *address,char *dbname,int port,char *dbuser,char *dbpwd);
	bool oleDb_Query(char *sqlStr,char* fields[],backfunc,void *handle,int *recordCount);
	bool oleDb_OneUpdate(char *sqlStr);
	bool oleDb_Update(char *sqlStr,bool isTrans);
	bool oleDb_UpdateArray(char *sqlStrs[],bool isTrans);
	char *oleDb_GetError(void);
	void oleDb_Close(void);	
	COleDbAccess(void);
	~COleDbAccess(void);
	// TODO: add your methods here.
};
#endif