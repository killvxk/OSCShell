#ifndef TRANSBUSI_H
#define TRANSBUSI_H

#include "Database.h"
/*
#define TYPE_ORACLE 0
#define TYPE_DB2    1
#define TYPE_MYSQL  2
*/

#define backfunc void (*func)(char *field,char *value,void *handle)

typedef struct{
	char dbtype[10];
	char address[20];
	char dbname[20];
	int  port;
	char provider[50];
	char dbuser[50];
	char dbpwd[50];
}CONNECT_STRUCT;

class TRANSBUSI{
private:
   char CnnStr[512];
   char ErrStr[1024];
   Database *db;
   bool isOpen;
   CONNECT_STRUCT connInfo;
public:    
	TRANSBUSI(char *dbtype,char *address,char *dbname,int port,char *dbuser,char *dbpwd);
   ~TRANSBUSI();
	bool query(char *sqlStr,char* fields[],backfunc,void *handle,int *recordCount);
	bool update(char *sqlStr);
	bool update(char *sqlStr,bool isTrans);
	bool update(char *sqlStrs[],bool isTrans);	
	char *getError();
	bool getStatus();
	void beginTrans();
	void commitTrans();
	void rollbackTrans();
};

#endif