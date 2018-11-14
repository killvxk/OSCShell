// LDap.cpp : 定义控制台应用程序的入口点。
//

#include <stdlib.h>
#include <ldap.h>
#include <vector>
using namespace std;

#ifndef MAX
# define MAX(a,b) (((a)>(b))?(a):(b))
# define MIN(a,b) (((a)<(b))?(a):(b))
#endif

char *split_url(char *url,char separator,size_t url_len)
{	
	int point;
	int size;
	int equal=0;
	if(url_len==0){
		size=strlen(url);
	}else{
		size=MIN(strlen(url),url_len);
	}
	for(point=size-2;point>0;point--){
		if(url[point]==separator)
		{
			equal=point;
        }
	}
	if(equal==0)
		return 0;
	url[equal]=0;
	return url+equal+1;
}

struct MultiAttrs 
{
	char szAttrs[][255];
};
typedef vector<MultiAttrs> vecAttrs;

int QueryLdap(char *szServer,char *szBind,char *szPwd,int port,char *searchBase,char *szQuery,char szAttrs[][255],vecAttrs &vecAttr,int argc)
{
	LDAP            *ld;
	LDAPMessage     *res, *e;
	int             i;
	char            *a, *dn;
	BerElement      *ptr;
	char            **vals;
	vecAttr.clear();

	/* open a connection */
	if ( (ld = ldap_open(szServer, port ))== NULL )
	{
		//exit( 1 );
		return -1;
	}

	/* authenticate as nobody */
	if ( ldap_simple_bind_s( ld, szBind, szPwd ) != LDAP_SUCCESS ) 
	{
		ldap_perror( ld, "ldap_simple_bind_s" );
	/* close and free connection resources */
	ldap_unbind( ld );
		return -1;
	}
	char **pAttrs = (char **)malloc(sizeof(char *) * (argc+1));
	for (i=0;i<argc;i++)
	{
		pAttrs[i] = szAttrs[i];
	}
	pAttrs[i] = NULL;

	if ( ldap_search_s( ld, searchBase,LDAP_SCOPE_SUBTREE,szQuery, pAttrs, 0, &res )
		!= LDAP_SUCCESS ) {
			ldap_perror( ld, "ldap_search_s" );
			//exit( 1 );
			free(pAttrs);
	/* free the search results */
	ldap_msgfree( res );
	/* close and free connection resources */
	ldap_unbind( ld );

			return -1;
	}
	//释放内存
	free(pAttrs);

	int ret = 0;

	/* step through each entry returned */
	for ( e = ldap_first_entry( ld, res ); e != NULL;
		e = ldap_next_entry( ld, e ) ) {
			/* print its name */
			dn = ldap_get_dn( ld, e );
			//printf( "dn: %s", dn );

			/* print each attribute */
			for ( a = ldap_first_attribute( ld, e, &ptr );
				a != NULL;
				a = ldap_next_attribute( ld, e, ptr ) ) {
					//printf( "\nattribute: %s\n", a );

					/* print each value */
					vals = ldap_get_values( ld, e, a );
					if(vals != NULL)
					{	
						if(vals[0] != NULL)
						{
							//遍历
							for (int i=0;i<argc;i++)
							{
								if(stricmp(szAttrs[i],a) == 0)
								{
									strcpy(szAttrs[i],vals[0]);
									ret = 1;
									break;
								}
							}
						}

					}
					ldap_value_free( vals );
			}
	}
	/* free the search results */
	ldap_msgfree( res );
	/* close and free connection resources */
	ldap_unbind( ld );
	return ret;
}

//localhost,cn=root,1,"cn=apps,cn=configs,dc=idm,dc=com",(cn=15591428231241)
int QueryLdap(char *szServer,char *szBind,char *szPwd,int port,char *searchBase,char *szQuery,char szAttrs[][255],int type,int argc)
{
	LDAP            *ld;
	LDAPMessage     *res, *e;
	int             i;
	char            *a, *dn;
	BerElement      *ptr;
	char            **vals;

	/* open a connection */
	if ( (ld = ldap_open(szServer, port ))== NULL )
	{
		//exit( 1 );
		return -1;
	}

	/* authenticate as nobody */
	if ( ldap_simple_bind_s( ld, szBind, szPwd ) != LDAP_SUCCESS ) 
	{
		ldap_perror( ld, "ldap_simple_bind_s" );
		//exit( 1 );
	/* close and free connection resources */
	ldap_unbind( ld );

		return -1;
	}
	char **pAttrs = (char **)malloc(sizeof(char *) * (argc+1));
	for (i=0;i<argc;i++)
	{
		pAttrs[i] = szAttrs[i];
	}
	pAttrs[i] = NULL;

	if ( ldap_search_s( ld, searchBase,LDAP_SCOPE_SUBTREE,szQuery, pAttrs, 0, &res )
		!= LDAP_SUCCESS ) {
			ldap_perror( ld, "ldap_search_s" );
			//exit( 1 );
			free(pAttrs);
	/* free the search results */
	ldap_msgfree( res );
	/* close and free connection resources */
	ldap_unbind( ld );

			return -1;
	}
	//释放内存
	free(pAttrs);
	
	int ret = 0;

	/* step through each entry returned */
	for ( e = ldap_first_entry( ld, res ); e != NULL;
		e = ldap_next_entry( ld, e ) ) {
			/* print its name */
			dn = ldap_get_dn( ld, e );
			//printf( "dn: %s", dn );
			
			/* print each attribute */
			for ( a = ldap_first_attribute( ld, e, &ptr );
				a != NULL;
				a = ldap_next_attribute( ld, e, ptr ) ) {
					//printf( "\nattribute: %s\n", a );

					/* print each value */
					vals = ldap_get_values( ld, e, a );
					if(vals != NULL)
					{	
						//多值查询 如果是suExtAttr 字段
						if(type >= 0 && stricmp(a,"suExtAttr") == 0)
						{ 
							char szType[2] = {0};
							itoa(type,szType,10);
							for (int j=0;j<argc;j++)
							{
								if(stricmp(szAttrs[j],a) == 0)
								{
									for (int i = 0; vals[i] != NULL; i++ ) 
									{
										//拆分
										char *sztemp =  split_url(vals[i],'=',strlen(vals[i]));
										//比较
										if(stricmp(vals[i],szType) == 0)
										{
											strcpy(szAttrs[j],sztemp);
											ret = 1;
											break;
										}
									}
									break;
								}
							}	
						}
						else
						{
							if(vals[0] != NULL)
							{
								//遍历
								for (int i=0;i<argc;i++)
								{
									if(stricmp(szAttrs[i],a) == 0)
									{
										strcpy(szAttrs[i],vals[0]);
										ret = 1;
										break;
									}
								}
							}
						}
					}
					ldap_value_free( vals );
			}
	}
	/* free the search results */
	ldap_msgfree( res );
	/* close and free connection resources */
	ldap_unbind( ld );
	return ret;
}

//多值查询
char **MultiAttQueryLdap(char *szServer,char *szBind,char *szPwd,int port,char *searchBase,char *szQuery,char **szAttrs)
{
	LDAP            *ld;
	LDAPMessage     *res, *e;
	int             i;
	char            *a, *dn;
	BerElement      *ptr;
	char            **vals;

	/* open a connection */
	//fprintf(stderr,"\n%s,%d",szServer, LDAP_PORT);
	if ( (ld = ldap_open(szServer, port))== NULL )
	{
		//exit( 1 );
		return NULL;
	}

	/* authenticate as nobody */
	//fprintf(stderr,"\n%s,%s",szBind, szPwd);
	if ( ldap_simple_bind_s( ld, szBind, szPwd ) != LDAP_SUCCESS ) 
	{
		ldap_perror( ld, "ldap_simple_bind_s" );

		/* close and free connection resources */
		ldap_unbind( ld );
		return NULL;
	}

	//fprintf(stderr,"\n%s,%s",searchBase, szQuery);
	if ( ldap_search_s( ld, searchBase,LDAP_SCOPE_SUBTREE,szQuery, szAttrs, 0, &res )
		!= LDAP_SUCCESS ) {
			ldap_perror( ld, "ldap_search_s" );
			/* free the search results */
			ldap_msgfree( res );
			/* close and free connection resources */
			ldap_unbind( ld );
			return NULL;
	}

	//char *szValues[301];
	char **szValues=(char**)malloc(sizeof(char *)*(201));
	int index = 0;

	/* step through each entry returned */
	for ( e = ldap_first_entry( ld, res ); e != NULL; e = ldap_next_entry( ld, e ) ) {

			if(index == 200)
				break;

			/* print its name */
			dn = ldap_get_dn( ld, e );
			//printf( "dn: %s", dn );

			/* print each attribute */
			int count =0;
			for ( a = ldap_first_attribute( ld, e, &ptr );
				a != NULL;
				a = ldap_next_attribute( ld, e, ptr ) ) {

					if(index == 300)
						break;

					//printf( "\nattribute: %s\n", a );
					/* print each value */
					vals = ldap_get_values( ld, e, a );
					int i = 0;
					//char **szValues=malloc(sizeof(char *)*(argc+1));
					if(vals != NULL)
					{	
						//过滤
						if(strnicmp(vals[i],"LIMITCMD",8)==0){
						   ldap_value_free( vals );
						   continue;
						}

						//多值查询
						for (i = 0; vals[i] != NULL; i++ ) 
						{
							if(i == 200)
							{
								break;
							}
							char *szVal = (char*)malloc(1024);
							strcpy(szVal,vals[i]);
							szValues[index++] = szVal;
						}
						ldap_value_free( vals );
					}
			}
	}

	szValues[index] = NULL;

	/* free the search results */
	ldap_msgfree( res );
	/* close and free connection resources */
	ldap_unbind( ld );
	return szValues;
}

int AddLdap(char *szServer,char *szBind,char *szPwd,int port,char *addDn,char *addValue)
{
	LDAP            *ld;
	LDAPMessage     *res, *e;
	int             i;
	char            *a, *dn;
	BerElement      *ptr;
	char            **vals;
	LDAPMod *attrs[2];
	LDAPMod attr;

	/* open a connection */
	if ( (ld = ldap_open(szServer, port ))== NULL )
	{
		return -1;
	}

	/* authenticate as nobody */
	if ( ldap_simple_bind_s( ld, szBind, szPwd ) != LDAP_SUCCESS ) 
	{
		ldap_perror( ld, "ldap_simple_bind_s" );
		ldap_unbind( ld );
		return -1;
	}

	char *suExtAttr_values[2];
	suExtAttr_values[0]=addValue;
	suExtAttr_values[1]=NULL;
	attr.mod_op = LDAP_MOD_ADD;
	attr.mod_type = "suExtAttr";
	attr.mod_values = suExtAttr_values;
	attrs[0]=&attr;
	attrs[1]=NULL;
	if(ldap_modify_s(ld,addDn,attrs)!= LDAP_SUCCESS ) {
		ldap_perror( ld, "ldap_add_s" );
		ldap_unbind( ld );
		return -1;   
	}

	/* close and free connection resources */
	ldap_unbind( ld );
	return 0;
}

int DeleteLdap(char *szServer,char *szBind,char *szPwd,int port,char *deleteDn)
{
	LDAP            *ld;
	LDAPMessage     *res, *e;
	int             i;
	char            *a, *dn;
	BerElement      *ptr;
	char            **vals;
	
	/* open a connection */
	if ( (ld = ldap_open(szServer, port ))== NULL )
	{
		return -1;
	}

	/* authenticate as nobody */
	if ( ldap_simple_bind_s( ld, szBind, szPwd ) != LDAP_SUCCESS ) 
	{
		ldap_perror( ld, "ldap_simple_bind_s" );
   	    ldap_unbind( ld );
		return -1;
	}

   if(ldap_delete_s(ld,deleteDn)!= LDAP_SUCCESS ) {
	    ldap_perror( ld, "ldap_search_s" );
	    ldap_unbind( ld );
		return -1;   
   }

	/* close and free connection resources */
	ldap_unbind( ld );
	return 0;
}

