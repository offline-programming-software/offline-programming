#pragma once
#include "utils.h"

namespace utils {
	QStringList BSTR2QStringList(BSTR bstr)
	{

		QStringList list;

		// 1?? ÅÐ¶Ï bstr ÊÇ·ñÎª¿Õ
		if (!bstr || SysStringLen(bstr) == 0)
			return list;

		// 2?? BSTR ¡ú QString
		QString str = QString::fromWCharArray(bstr, SysStringLen(bstr));
		list = str.split("#", QString::SkipEmptyParts);


		return list;
	}


}

