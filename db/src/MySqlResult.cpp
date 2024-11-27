#include "MySqlResult.h"

#include <iostream>

namespace flaw {
	MySqlResult::MySqlResult(boost::mysql::results& results)
		: _results(results)
	{
		
	}
}