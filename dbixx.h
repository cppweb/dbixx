#ifndef _DBIXX_H_
#define _DBIXX_H_

#include <dbi/dbi.h>
#include <stdexcept>
#include <ctime>
#include <map>
#include <vector>
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
private:
	row(row const &);
	row const &operator=(row const &);
public:
	row() { current=0; owner=false; res=NULL; };
	~row();
	dbi_result get_dbi_result() { return res; };
	void set(dbi_result &r);
	void reset();
	bool isempty();
	void assign(dbi_result &r);
	bool isnull(int inx);
	bool isnull(std::string const &id);
	bool fetch(int pos,int &value);
	bool fetch(int pos,unsigned &value);
	bool fetch(int pos,long long &value);
	bool fetch(int pos,unsigned long long &value);
	bool fetch(int pos,double &value);
	bool fetch(int pos,std::string &value);
	bool fetch(int pos,std::tm &value);
	bool next();
	// Sugar
	bool operator[](std::string const & id) { return isnull(id); };
	bool operator[](int ind) { return isnull(ind); };
	template<typename T>
	row &operator>>(T &v) { current++; fetch(current,v); return *this; };
	unsigned int cols();
};

namespace details {
template<typename C,typename T>
struct result_binder;
}

class result
{
	dbi_result res;
private:
	result(result const &);
	result const &operator=(result const &);
public:
	result() : res(NULL) {};
	~result();
	void assign(dbi_result r);
	unsigned long long rows();
	unsigned int cols();
	bool next(row &r);
	template<typename C>
	details::result_binder<C,typename C::value_type> bind(C &c)
	{
		return details::result_binder<C,typename C::value_type>(c,*this);
	}
};

namespace details {

template<typename T>
struct basic_row_binder
{
	virtual void bind(row &r,T &p)
	{ throw dbixx_error("Direct use of basic_row_binder"); };
	virtual ~basic_row_binder(){}
};

template<typename T,typename M>
struct row_binder : public basic_row_binder<T>
{
	M T::*m_;
	row_binder(M T::*m) : m_(m) {}
	virtual void bind(row &r,T &p) {
		r>>(p.*m_);
	}
};

template<typename C,typename T>
struct result_binder {
	C &collection;
	result &res;
	unsigned cols;
	typedef std::vector<basic_row_binder<T> *> members_t;
	members_t members;
	result_binder(C &c,result &r) :
		collection(c),
		res(r),
		cols(r.cols())
	{
		members.reserve(cols);
	}
	~result_binder()
	{
		typename members_t::iterator p;
		for(p=members.begin();p!=members.end();++p)
		{
			delete *p;
		}
	}
	template<typename M>
	result_binder<C,T> &operator<<(M T::*p)
	{
		members.push_back(new row_binder<T,M>(p));
		if(members.size()==cols) {
			run();
		}
		return *this;
	}
	void run()
	{
		collection.resize(res.rows());
		typename members_t::iterator c;
		row r;
		for(typename C::iterator p=collection.begin();res.next(r);++p) {
			for(c=members.begin();c!=members.end();++c) {
				(*c)->bind(r,*p);
			}
		}
	}
};

} // namespace details


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
