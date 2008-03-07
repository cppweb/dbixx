#include "dbixx.h"
#include <iostream>
using namespace dbixx;
using namespace std;

int main()
{
//	session sql("sqlite3");
//	sql.param("dbname","test.db");
//	sql.param("sqlite3_dbdir","./");

	session sql("mysql");
	sql.param("dbname","cppcms");
	sql.param("username","root");
	sql.param("password","root");

	sql.connect();
	
	sql<<"drop table if exists test",
		exec();
	sql<<"create table test ( id integer,n integer, f real , t timestamp ,name text )",
		exec();
	std::tm t;
	time_t tt;
	tt=time(NULL);
	localtime_r(&tt,&t);

	
	int n;
	sql<<"insert into test values(?,?,?,?,?)",
		1,10,3.1415926565,t,"Hello \'World\'",
		exec();
	sql<<"insert into test values(?,?,?,?,?)",
		2,use(10,true),use(3.1415926565,true),use(t,true),use("Hello \'World\'",true),
		exec();

	row r;
	result res;
	sql<<"select id,n,f,t,name from test limit 10",
		res;
	n=0;
	while(res.next(r)){
		double f=-1;
		int id=-1,k=-1;
		std::tm atime={0};
		string name="nonset";
		r >> id >> k >> f >> atime >> name;
		cout <<id << ' '<<k <<' '<<f<<' '<<name<<' '<<asctime(&atime)<<endl;
		n++;
	}
	return 0;
}
