//
// Created by 0060328 on 25-9-18.
//

#ifndef GLESHEADER_H
#define GLESHEADER_H
#if defined(__has_include)
#if __has_include(<GLES3/gl31.h>)
#include <GLES3/gl31.h>
#elif __has_include(<GLES3/gl3.h>)
#include <GLES3/gl3.h>
#elif __has_include(<GLES2/gl2.h>)
#include <GLES2/gl2.h>
#else
#error "No supported GLES header found"
#endif
#if __has_include(<GLES3/gl3ext.h>)
#include <GLES3/gl3ext.h>
#endif
#if __has_include(<GLES2/gl2ext.h>)
#include <GLES2/gl2ext.h>
#endif
#else
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#endif
#endif //GLESHEADER_H
