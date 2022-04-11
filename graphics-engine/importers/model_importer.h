#ifndef MODEL_IMPORTER_INCLUDED
#define MODEL_IMPORTER_INCLUDED

#include <string>

namespace dengine
{
	class IModelImporter{
	public:
		virtual ~IModelImporter() = default;
		virtual void Import(std::pmr::string path) = 0;
	};
	
}

#endif

