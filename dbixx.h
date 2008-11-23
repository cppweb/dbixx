#ifndef _DBIXX_H_
#define _DBIXX_H_

#include <dbi/dbi.h>
#include <stdexcept>
#include <ctime>
#include <map>
#include <iostream>

namespace dbixx {

class dbixx_error : public std::runtime_error
{
	char query_[256];
public:
	char const *query() const { return query_; };
	dbixx_error(std::string const &error,std::string const &q="") : std::runtime_error(error)
	{
		strncpy(query_,q.c_str(),256);
	};
};

class row 
{
	dbi_result res;
	bool owner;
	int current;
	void check_set();

	void set(dbi_result &r);
	void reset();
	void assign(dbi_result &r);
	bool next();
	friend class session;
	friend class result;
private:
	row(row const &);
	row const &operator=(row const &);
public:
	row() { current=0; owner=false; res=NULL; };
	~row();
	dbi_result get_dbi_result() { return res; };
	bool isempty();
	bool isnull(int inx);
	bool isnull(std::string const &id);
	bool fetch(int pos,int &value);
	bool fetch(int pos,unsigned &value);
	bool fetch(int pos,long long &value);
	bool fetch(int pos,unsigned long long &value);
	bool fetch(int pos,double &value);
	bool fetch(int pos,std::string &value);
	bool fetch(int pos,std::tm &value);
	// Sugar
	bool operator[](std::string const & id) { return isnull(id); };
	bool operator[](int ind) { return isnull(ind); };
	template<typename T>
	row &operator>>(T &v) { current++; fetch(current,v); return *this; };
	unsigned int cols();
};

class result
{
	dbi_result res;
	void assign(dbi_result r);
	friend class session;
private:
	result(result const &);
	result const &operator=(result const &);
public:
	result() : res(NULL) {};
	~result();
	unsigned long long rows();
	unsigned int cols();
	bool next(row &r);
};

/* Auxilary types/functions for syntactic sugar */

struct null {};
struct exec {};

template<typename T>
std::pair<T,bool> use(T ref, bool isnull=false)
{
	return std::pair<T,bool>(ref,isnull);
}


class session {
	std::string query_in;
	unsigned pos_read;
	std::string escaped_query;
	unsigned pos_write;
	bool ready_for_input;
	bool complete;
	unsigned long long affected_rows;


	std::string backend;
	dbi_conn conn;
	std::map<std::string,std::string> string_params; 
	std::map<std::string,int> numeric_params; 
	void check_open();
	void error();
	void escape();
	void check_input();
private:
	session(session const &);
	session const &operator=(session const &);
public:
	session();
	session(std::string const &backend);
	~session();
	void driver(std::string const &backend);
	void param(std::string const &par,std::string const &val);
	void param(std::string const &par,int);
	void connect();
	void reconnect();
	void close();
	dbi_conn get_dbi_conn() { return conn; };


	void query(std::string const &query);
	unsigned long long rowid(char const *seq=NULL);
	unsigned long long affected() { return affected_rows ;};

	void bind(std::string const &s,bool isnull=false);
	void bind(long long int const &v,bool isnull=false);
	void bind(unsigned long long const &v,bool isnull=false);
	void bind(int const &v,bool isnull=false) { bind((long long)v,isnull); };
	void bind(unsigned const &v,bool isnull=false) { bind((unsigned long long)v,isnull); };
	void bind(double const &v,bool isnull=false);
	void bind(std::tm const &time,bool isnull=false);
	void bind(null const &,bool isnull=true);

	void exec();

	void fetch(result &res);

	bool single(row &r);

	/* Syntactic sugar */
	
	session &operator<<(std::string const &q) { query(q); return *this; };

	template<typename T>
	session &operator,(T v) { bind(v,false); return *this; };

	template<typename T>
	session &operator,(std::pair<T,bool> p) { bind(p.first,p.second); return *this; }

	void operator,(dbixx::exec const &e) { exec(); };
	void operator,(result &res) { fetch(res); };
	bool operator,(row &r) { return single(r); };
};

class transaction {
	session &sql;
	bool commited;
	void begin();
private:
	transaction(transaction const &);
	transaction const &operator=(transaction const &);
public:
	transaction(session &s) : sql(s) { commited=false; begin(); }
	void commit();
	void rollback();
	~transaction();
};

}

#endif // _DBIXX_H_
