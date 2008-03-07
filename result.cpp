#include "dbixx.h"

namespace dbixx {
using namespace std;

result::~result()
{
	if(res)
		dbi_result_free(res);
}

void result::assign(dbi_result r)
{
	if(res && r!=res)
		dbi_result_free(res);
	res=r;
}

bool result::next(row &r)
{
	if(dbi_result_next_row(res)) {
		r.set(res);
		return true;
	}
	else {
		r.reset();
		return false;
	}
}
}
