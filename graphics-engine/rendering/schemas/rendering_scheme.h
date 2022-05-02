#ifndef RENDERING_SCHEME_INCLUDED
#define RENDERING_SCHEME_INCLUDED


namespace dengine
{
	class IRenderingScheme{
	public:
		virtual ~IRenderingScheme() = default;
		virtual unsigned int LoadShaderProgram() = 0;
	};
}

#endif

