/*
 * fg_geometry.c
 *
 * Freeglut geometry rendering methods.
 *
 * Copyright (c) 1999-2000 Pawel W. Olszta. All Rights Reserved.
 * Written by Pawel W. Olszta, <olszta@sourceforge.net>
 * Creation date: Fri Dec 3 1999
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PAWEL W. OLSZTA BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <nds.h>

#define _DECLARE_INTERNAL_DRAW_DO_DECLARE(name,nameICaps,nameCaps,vertIdxs)\
    static void fgh##nameICaps( GLboolean useWireMode )\
    {\
        if (!name##Cached)\
        {\
            fgh##nameICaps##Generate();\
            name##Cached = GL_TRUE;\
        }\
        \
        if (useWireMode)\
        {\
            fghDrawGeometryWire (name##_verts,name##_norms,nameCaps##_VERT_PER_OBJ, \
                                 NULL,nameCaps##_NUM_FACES,nameCaps##_NUM_EDGE_PER_FACE,GL_LINE_LOOP,\
                                 NULL,0,0);\
        }\
        else\
        {\
            fghDrawGeometrySolid(name##_verts,name##_norms,NULL,nameCaps##_VERT_PER_OBJ,\
                                 vertIdxs, 1, nameCaps##_VERT_PER_OBJ_TRI); \
        }\
    }
#define DECLARE_INTERNAL_DRAW(name,nameICaps,nameCaps)                        _DECLARE_INTERNAL_DRAW_DO_DECLARE(name,nameICaps,nameCaps,NULL)
#define DECLARE_INTERNAL_DRAW_DECOMPOSED_TO_TRIANGLE(name,nameICaps,nameCaps) _DECLARE_INTERNAL_DRAW_DO_DECLARE(name,nameICaps,nameCaps,name##_vertIdxs)
