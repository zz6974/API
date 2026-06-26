// nanosvg.h - public domain SVG parsing library - https://github.com/memononen/nanosvg
// by Mikko Mononen
//
// This is a single-file library for parsing SVG files.
// It parses SVG files and converts them to a simple path-based representation.
//
// Usage:
//
// NSVGimage* nsvgParse(const char* svg, const char* units, float dpi);
//    - parses SVG from a string
//    - svg is the SVG data
//    - units is the units to use ("px", "pt", "pc", "mm", "cm", "in")
//    - dpi is the DPI to use for conversion
//    - returns NULL on failure
//
// void nsvgDelete(NSVGimage* image);
//    - frees the SVG image
//
// -----------------------------------------------------------------------------
//
// LICENSE
//
// This software is in the public domain. Where that dedication is not recognized, you are granted a perpetual, irrevocable license to copy, distribute, and modify this file as you see fit.
//
// -----------------------------------------------------------------------------
//
// Please note that nanosvg.h is a single-file library, which means that it's designed to be included directly into your project. This file is just a placeholder to satisfy the include statement.
// For the actual implementation, you should download the latest version from https://github.com/memononen/nanosvg/blob/master/src/nanosvg.h
