// This implements support common to all glGet functions.
//
// Copyright (c) 2018 Riverbank Computing Limited <info@riverbankcomputing.com>
// 
// This file is part of PyQt5.
// 
// This file may be used under the terms of the GNU General Public License
// version 3.0 as published by the Free Software Foundation and appearing in
// the file LICENSE included in the packaging of this file.  Please review the
// following information to ensure the GNU General Public License version 3.0
// requirements will be met: http://www.gnu.org/copyleft/gpl.html.
// 
// If you do not wish to use this file under the terms of the GPL version 3.0
// then you may purchase a commercial license.  For more information contact
// info@riverbankcomputing.com.
// 
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.


#include "sipAPIQtGui.h"

#if defined(SIP_FEATURE_PyQt_OpenGL)

#include "qpyopengl_api.h"


#if QT_VERSION >= 0x050100

// Return the number of values returned for a particular parameter, or the
// subsequent query needed to obtain the number of values.
GLint qpyopengl_get(GLenum pname, GLenum *query)
{
    GLint nr_params = 0;

    switch (pname)
    {
#if defined(GL_COMPRESSED_TEXTURE_FORMATS)
    case GL_COMPRESSED_TEXTURE_FORMATS:
        *query = GL_NUM_COMPRESSED_TEXTURE_FORMATS;
        break;
#endif

#if defined(GL_PROGRAM_BINARY_FORMATS)
    case GL_PROGRAM_BINARY_FORMATS:
        *query = GL_NUM_PROGRAM_BINARY_FORMATS;
        break;
#endif

#if defined(GL_SHADER_BINARY_FORMATS)
    case GL_SHADER_BINARY_FORMATS:
        *query = GL_NUM_SHADER_BINARY_FORMATS;
        break;
#endif

#if defined(GL_COLOR_MATRIX)
    case GL_COLOR_MATRIX:
#endif
#if defined(GL_MODELVIEW_MATRIX)
    case GL_MODELVIEW_MATRIX:
#endif
#if defined(GL_PROJECTION_MATRIX)
    case GL_PROJECTION_MATRIX:
#endif
#if defined(GL_TEXTURE_MATRIX)
    case GL_TEXTURE_MATRIX:
#endif
#if defined(GL_TRANSPOSE_COLOR_MATRIX)
    case GL_TRANSPOSE_COLOR_MATRIX:
#endif
#if defined(GL_TRANSPOSE_MODELVIEW_MATRIX)
    case GL_TRANSPOSE_MODELVIEW_MATRIX:
#endif
#if defined(GL_TRANSPOSE_PROJECTION_MATRIX)
    case GL_TRANSPOSE_PROJECTION_MATRIX:
#endif
#if defined(GL_TRANSPOSE_TEXTURE_MATRIX)
    case GL_TRANSPOSE_TEXTURE_MATRIX:
#endif
        nr_params = 16;
        break;

#if defined(GL_ACCUM_CLEAR_VALUE)
    case GL_ACCUM_CLEAR_VALUE:
#endif
#if defined(GL_BLEND_COLOR)
    case GL_BLEND_COLOR:
#endif
#if defined(GL_COLOR_CLEAR_VALUE)
    case GL_COLOR_CLEAR_VALUE:
#endif
#if defined(GL_COLOR_WRITEMASK)
    case GL_COLOR_WRITEMASK:
#endif
#if defined(GL_CURRENT_COLOR)
    case GL_CURRENT_COLOR:
#endif
#if defined(GL_CURRENT_RASTER_COLOR)
    case GL_CURRENT_RASTER_COLOR:
#endif
#if defined(GL_CURRENT_RASTER_POSITION)
    case GL_CURRENT_RASTER_POSITION:
#endif
#if defined(GL_CURRENT_RASTER_SECONDARY_COLOR)
    case GL_CURRENT_RASTER_SECONDARY_COLOR:
#endif
#if defined(GL_CURRENT_RASTER_TEXTURE_COORDS)
    case GL_CURRENT_RASTER_TEXTURE_COORDS:
#endif
#if defined(GL_CURRENT_SECONDARY_COLOR)
    case GL_CURRENT_SECONDARY_COLOR:
#endif
#if defined(GL_CURRENT_TEXTURE_COORDS)
    case GL_CURRENT_TEXTURE_COORDS:
#endif
#if defined(GL_FOG_COLOR)
    case GL_FOG_COLOR:
#endif
#if defined(GL_LIGHT_MODEL_AMBIENT)
    case GL_LIGHT_MODEL_AMBIENT:
#endif
#if defined(GL_MAP2_GRID_DOMAIN)
    case GL_MAP2_GRID_DOMAIN:
#endif
#if defined(GL_SCISSOR_BOX)
    case GL_SCISSOR_BOX:
#endif
#if defined(GL_VIEWPORT)
    case GL_VIEWPORT:
#endif
        nr_params = 4;
        break;

#if defined(GL_SMOOTH_POINT_SIZE_RANGE)
    case GL_SMOOTH_POINT_SIZE_RANGE:
#endif
#if defined(GL_POINT_SIZE_RANGE) && (!defined(GL_SMOOTH_POINT_SIZE_RANGE) || GL_SMOOTH_POINT_SIZE_RANGE != GL_POINT_SIZE_RANGE)
    case GL_POINT_SIZE_RANGE:
#endif

#if defined(GL_SMOOTH_LINE_WIDTH_RANGE)
    case GL_SMOOTH_LINE_WIDTH_RANGE:
#endif
#if defined(GL_LINE_WIDTH_RANGE) && (!defined(GL_SMOOTH_LINE_WIDTH_RANGE) || GL_SMOOTH_LINE_WIDTH_RANGE != GL_LINE_WIDTH_RANGE)
    case GL_LINE_WIDTH_RANGE:
#endif

#if defined(GL_ALIASED_POINT_SIZE_RANGE)
    case GL_ALIASED_POINT_SIZE_RANGE:
#endif
#if defined(GL_ALIASED_LINE_WIDTH_RANGE)
    case GL_ALIASED_LINE_WIDTH_RANGE:
#endif
#if defined(GL_DEPTH_RANGE)
    case GL_DEPTH_RANGE:
#endif
#if defined(GL_MAP1_GRID_DOMAIN)
    case GL_MAP1_GRID_DOMAIN:
#endif
#if defined(GL_MAP2_GRID_SEGMENTS)
    case GL_MAP2_GRID_SEGMENTS:
#endif
#if defined(GL_MAX_VIEWPORT_DIMS)
    case GL_MAX_VIEWPORT_DIMS:
#endif
#if defined(GL_POLYGON_MODE)
    case GL_POLYGON_MODE:
#endif
#if defined(GL_VIEWPORT_BOUNDS_RANGE)
    case GL_VIEWPORT_BOUNDS_RANGE:
#endif
        nr_params = 2;
        break;

    default:
        nr_params = 1;
    }

    return nr_params;
}

#endif

#endif
