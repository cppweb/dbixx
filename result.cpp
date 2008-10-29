#include "dbixx.h"

namespace dbixx {
using namespace std;

result::~result()
{
	if(res)
		dbi_result_free(res);
}

unsigned long long result::rows()
{
	if(res)
		return dbi_result_get_numrows(res);
	throw dbixx_error("No result assigned");
}

void result::assign(dbi_result r)
{
	if(res && r!=res)
		dbi_result_free(res);
	res=r;
}

bool result::next(row &r)
{
	if(!res)
		throw dbixx_error("No result assigned");
	if(dbi_result_next_row(res)) {
		r.set(res);
		return true;
	}
	else {
		r.reset();
		return false;
	}
}

unsigned int result::cols()
{
	unsigned c;
	if(res && (c=dbi_result_get_numfields(res))!=DBI_FIELD_ERROR) {
		return c;
	}
	throw dbixx_error("Failed to fetch number of columns");
}


}
