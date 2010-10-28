#!/usr/bin/env python
#
# Copyright (C) 2010 Luka Cehovin (luka.cehovin@gmail.com)
# 
# genesx is free software.
# 
# You may redistribute it and/or modify it under the terms of the
# GNU General Public License, as published by the Free 
# Software Foundation; either version 2 of the License, 
# or (at your option) any later version.
# 
# genesx is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public 
# License along with the source. If not, write to:
# The Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor
# Boston, MA  02110-1301, USA.
#
# ------------------------------------------------------------
#
# Check the README file for more details!

from genesx import *

start = Element("", "_start", "", "")
start.add_child(Child("image", 1, 1))

image = Element("", "image", "none", "Root tag for stack")
image.add_argument("w", "int", True)
image.add_argument("h", "int", True)
image.add_argument("name", "text", False)
image.add_child(Child("stack", 1, 1))

stack = Element("", "stack", "none", "Stack element")
stack.add_argument("x", "int", False)
stack.add_argument("y", "int", False)
stack.add_argument("name", "text", False)
stack.add_child(Child("layer"))
stack.add_child(Child("stack"))

layer = Element("", "layer", "none", "Layer element")
layer.add_argument("x", "int", False)
layer.add_argument("y", "int", False)
layer.add_argument("name", "text", False)
layer.add_argument("src", "text", True)
layer.add_argument("opacity", "float", False)
layer.add_argument("visibility", "text", False)

elements = {"_start": start, "image" : image, "stack" : stack, "layer" : layer}

generate_parser("stack", "OpenRaster stack XML parser", elements, "c_expat", "../")
