#ifndef _DBIXX_H_
#define _DBIXX_H_

#include <dbi/dbi.h>
#include <stdexcept>
#include <ctime>
#include <map>
#include <cstring>

namespace dbixx {

///
/// \brief Exception throw in case of error using database
///
class dbixx_error : public std::runtime_error
{
public:
	///
	/// Get the query used in DB request
	///
	char const *query() const { return query_.c_str(); };
	///
	/// Create an exception object with \a error and a query string \a q
	///
	dbixx_error(std::string const &error,std::string const &q=std::string()) :
		std::runtime_error(error),
		query_()
	{
	}
	~dbixx_error() throw()
	{
	}
private:
	std::string query_;
};

///
/// \brief This class represents a single row that is fetched from the DB
///

class row 
{
	// non copyable
	row(row const &);
	row const &operator=(row const &);
public:
	/// 
	/// Creates an empty row
	/// 
	row() { current=0; owner=false; res=NULL; };
	~row();
	///
	/// Get underlying libdbi object. For low level access
	///
	dbi_result get_dbi_result() { return res; };
	///
	/// Check if this row has some data or not
	///
	bool isempty();
	///
	/// Check if the column at position \a indx has NULL value, first column index is 1
	///
	bool isnull(int inx);
	///
	/// Check if the column named \a id has NULL value
	///
	bool isnull(std::string const &id);
	///
	/// Fetch \a value at position \a pos (starting from 1), returns false if the column has null value.
	///
	bool fetch(int pos,short &value);
	///
	/// Fetch \a value at position \a pos (starting from 1), returns false if the column has null value.
	///
	bool fetch(int pos,unsigned short &value);
	///
	/// Fetch \a value at position \a pos (starting from 1), returns false if the column has null value.
	///
	bool fetch(int pos,int &value);
	///
	/// Fetch \a value at position \a pos (starting from 1), returns false if the column has null value.
	///
	bool fetch(int pos,unsigned &value);
	///
	/// Fetch \a value at position \a pos (starting from 1), returns false if the column has null value.
	///
	bool fetch(int pos,long  &value);
	///
	/// Fetch \a value at position \a pos (starting from 1), returns false if the column has null value.
	///
	bool fetch(int pos,unsigned long &value);
	///
	/// Fetch \a value at position \a pos (starting from 1), returns false if the column has null value.
	///
	bool fetch(int pos,long long &value);
	///
	/// Fetch \a value at position \a pos (starting from 1), returns false if the column has null value.
	///
	bool fetch(int pos,unsigned long long &value);
	///
	/// Fetch \a value at position \a pos (starting from 1), returns false if the column has null value.
	///
	bool fetch(int pos,float &value);
	///
	/// Fetch \a value at position \a pos (starting from 1), returns false if the column has null value.
	///
	bool fetch(int pos,double &value);
	///
	/// Fetch \a value at position \a pos (starting from 1), returns false if the column has null value.
	///
	bool fetch(int pos,long double &value);
	///
	/// Fetch \a string value at position \a pos (starting from 1), returns false if the column has null value.
	///
	bool fetch(int pos,std::string &value);
	///
	/// Fetch \a time value at position \a pos (starting from 1), returns false if the column has null value.
	///
	bool fetch(int pos,std::tm &value);
	///
	/// Syntactic sugar for isnull(id)
	///
	bool operator[](std::string const & id) { return isnull(id); };
	///
	/// Syntactic sugar for isnull(id)
	///
	bool operator[](int ind) { return isnull(ind); };
	///
	/// Fetch a value \a v from the row starting with first column. Each next call updates the internal pointer
	/// to next column in row
	///
	template<typename T>
	row &operator>>(T &v) { current++; fetch(current,v); return *this; };
	///
	/// Get number of columns in the row
	///
	unsigned int cols();

	///
	/// Fetch value by column. It fetches the value from column \a col (starting from 1)
	/// and returns it. If the column is null it throws dbixx_error
	///
	template<typename T>
	T get(int col)
	{
		T v;
		if(!fetch(col,v)) {
			throw dbixx_error("Null value fetch");
		}
		return v;
	}
private:
	template<typename T>
	bool ufetch(int post,T &v);
	template<typename T>
	bool sfetch(int post,T &v);

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
};

///
/// \brief This class holds query result and allows iterating over its rows
///
class result
{
	result(result const &);
	result const &operator=(result const &);
public:
	///
	/// Get internal result implementation
	///
	dbi_result get_dbi_result() { return res; };
	///
	/// Create empty result
	///
	result() : res(NULL) {};
	///
	/// Destroy result
	///
	~result();
	///
	/// Get number of rows in the returned result
	///
	unsigned long long rows();
	///
	/// Get number of columns in the returned result
	///
	unsigned int cols();
	///
	/// Fetch next row and store it into \a r. Returns false if no more rows remain.
	///
	bool next(row &r);
private:
	dbi_result res;
	void assign(dbi_result r);
	friend class session;
};


///
/// \brief Special type to bind a NULL value to column using operator,() - syntactic sugar
///
struct null {};
///
/// \brief Special type to start statement execution using operator,() - syntactic sugar
///
struct exec {};

///
/// \This function returns a pair of value and NULL flag allowing to bind conditional values easily
/// like
///
/// \code
///  sql<<"insert into foo values(?)" << use(my_int,my_int_is_null) ;
/// \endcode
///
template<typename T>
std::pair<T,bool> use(T ref, bool isnull=false)
{
	return std::pair<T,bool>(ref,isnull);
}

///
/// \brief Class that represents connection session
///
class session {
	// non copyable
	session(session const &);
	session const &operator=(session const &);
public:
	///
	/// Create unconnected session
	///
	session();
	///
	/// Create unconnected session that uses SQL backend \a backend or connection_string 
	/// see connect(std::string const &connection_string) function
	///
	/// Note, if the string is not in connection string format but rather as driver name format
	/// it does not initiate connection, otherwise it does initiate one.
	///
	/// Passing driver name as parameter rather then connection string is deprecated, and available for 
	/// backward compatibility
	///
	session(std::string const &backend_or_connection_string);
	///
	/// Destroy the session and close the connection
	///
	~session();
	///
	/// Connect to DB using "short-cut" connection string. 
	///
	/// The string format is following:
	///
	/// backend:key=value[;key=value]*
	///
	/// where value is ether quoted string starting and ending with "'"
	/// or any other character sequence that does not include ";".
	/// If the value is looks like number (i.e. consists of digits and may start with "-"
	/// it is used as numeric value, if you want to prevent this use double quote.
	///
	/// For example:
	///
	/// sql.connect("sqlite3:dbname=test.db;sqlite3_dbdir=./");
	///
	/// is interpreted as sequence:
	///
	/// - sql.driver("sqlite3");
	/// - sql.param("dbname","test.db"); 
	/// - sql.param("sqlite3_dbdir","./");
	/// - sql.connect();
	///
	/// If you want to have a single quote in the string escape it as double quote:
	/// "mysql:username='foo';password='xdfr''s!d;f'" And the password would be interpreted as 
	/// "xdf's!d;f"
	///
	void connect(std::string const &connection_string);
	///
	/// Set driver, for example mysql
	///
	void driver(std::string const &backend);
	///
	/// Get current driver, for example "mysql", if driver was not set, empty string is returned
	///
	std::string driver();
	///
	/// Set string parameter
	///
	void param(std::string const &par,std::string const &val);
	///
	/// Set numeric parameter
	///
	void param(std::string const &par,int);
	///
	/// Connect to database
	///
	void connect();
	///
	/// Reconnect to database, useful if the DB was disconnected
	///
	void reconnect();
	///
	/// Close connection
	///
	void close();
	
	///
	/// Get low level libdbi connection object
	///
	dbi_conn get_dbi_conn() { return conn; };


	///
	/// Set query to be prepared for execution, parameters that should be binded
	/// should be marked with "?" symbol. Note: when binding string you should not
	/// write quites around it as they would be automatically added.
	///
	/// For example 
	///
	/// \code
	///  sql.query("SELECT * FROM users WHERE name=?")
	///  sql.bind(username);
	///  sql.fetch(res);
	/// \endcode
	///
	void query(std::string const &query);
	///
	/// Get last inserted rowid for sequence \a seq. Some DB require sequence name (postgresql)
	/// for other seq is just ignored (mysql, sqlite).
	///
	unsigned long long rowid(char const *seq=NULL);

	///
	/// Get number of affected rows by the last statement
	///
	unsigned long long affected() { return affected_rows ;};

	///
	/// Bind a string parameter at next position in query
	///
	void bind(std::string const &s,bool isnull=false);
	///
	/// Bind a numeric parameter at next position in query
	///
	void bind(int v,bool isnull=false);
	///
	/// Bind a numeric parameter at next position in query
	///
	void bind(unsigned v,bool isnull=false);
	///
	/// Bind a numeric parameter at next position in query
	///
	void bind(long v,bool isnull=false);
	///
	/// Bind a numeric parameter at next position in query
	///
	void bind(unsigned long v,bool isnull=false);
	///
	/// Bind a numeric parameter at next position in query
	///
	void bind(long long int v,bool isnull=false);
	///
	/// Bind a numeric parameter at next position in query
	///
	void bind(unsigned long long v,bool isnull=false);
	///
	/// Bind a numeric parameter at next position in query
	///
	void bind(double v,bool isnull=false);
	///
	/// Bind a numeric parameter at next position in query
	///
	void bind(long double v,bool isnull=false);
	///
	/// Bind a date-time parameter at next position in query
	///
	void bind(std::tm const &time,bool isnull=false);
	///
	/// Bind a NULL parameter at next position in query, \a isnull is just for consistency, don't use it.
	///
	void bind(null const &,bool isnull=true);

	///
	/// Execute the statement
	///
	void exec();

	///
	/// Fetch query result into \a res
	///
	void fetch(result &res);

	///
	/// Fetch a single row from query. If no rows where selected returns false,
	/// if more then one row is available, throws dbixx_error, If exactly one 
	/// row was fetched, returns true.
	///
	bool single(row &r);

	///
	/// Syntactic sugar for query(q)
	///
	session &operator<<(std::string const &q) { query(q); return *this; };
	///
	/// Syntactic sugar for bind(v)
	///	
	session &operator,(std::string const &v) { bind(v,false); return *this; };
	///
	/// Syntactic sugar for bind(p.first,p.second), usually used with use() function
	///	
	session &operator,(std::pair<std::string,bool> p) { bind(p.first,p.second); return *this; }
	///
	/// Syntactic sugar for bind(v)
	///	
	session &operator,(char const *v) { bind(v,false); return *this; };
	///
	/// Syntactic sugar for bind(v)
	///	
	session &operator,(std::tm const &v) { bind(v,false); return *this; };
	
	///
	/// Syntactic sugar for calling exec() function.
	///	
	void operator,(dbixx::exec const &e) { exec(); };
	///
	/// Syntactic sugar for fetching result - calling fetch(res)
	///	
	void operator,(result &res) { fetch(res); };
	///
	/// Syntactic sugar for fetching a single row - calling single(r)
	///	
	bool operator,(row &r) { return single(r); };

	///
	/// Syntactic sugar for bind(v)
	///	
	template<typename T>
	session &operator,(T v) { bind(v,false); return *this; };

	///
	/// Syntactic sugar for bind(p.first,p.second), usually used with use() function
	///	
	template<typename T>
	session &operator,(std::pair<T,bool> p) { bind(p.first,p.second); return *this; }
private:
	template<typename T>
	void do_bind(T v,bool);

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

};

///
/// \brief Transaction scope guard.
///
/// It automatically rollbacks the transaction during stack unwind
/// if it wasn't committed
///
class transaction {
	// non copyable
	transaction(transaction const &);
	transaction const &operator=(transaction const &);
public:
	///
	/// Begin transaction on session \a s
	///
	transaction(session &s) : sql(s) { commited=false; begin(); }
	///
	/// Commit translation to database 
	///
	void commit();
	///
	/// Rollbacks the transaction
	///
	void rollback();
	///
	/// If commit wasn't called, calls rollback()
	///
	~transaction();
private:
	session &sql;
	bool commited;
	void begin();
};

}

#endif // _DBIXX_H_
