
#ifndef SFRAME_CSV_H
#define SFRAME_CSV_H

#include "Table.h"

namespace sframe {

class CSV
{
public:

	// ½âÎö
	static bool Parse(const std::string & content, Table & tbl);

};

}

#endif
