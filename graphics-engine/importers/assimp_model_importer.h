#ifndef ASSIMP_MODEL_IMPORTER_INCLUDED
#define ASSIMP_MODEL_IMPORTER_INCLUDED

#include <importers/model_importer.h>

namespace dengine{
	class AssimpModelImporter : public IModelImporter {
	public:
		void Import(std::pmr::string path) override;
	};
	
}


#endif
