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

import sys, os.path
from optparse import OptionParser
import re
from Cheetah.Template import Template

class Argument:
    def __init__(self, name, type, mandatory):
        self.name = name
        self.type = type
        self.mandatory = mandatory

class Automata:
    def __init__(self):
        self.states = list()
        self.transitions = list()
        self.final = list()

    def add_state(self, name, final=False):
        self.states.append(name)
        if final:
            self.final.append(name)

    def add_transition(self, start, end, letter):
        self.transitions.append((start, end, letter))

class Child:
    def __init__(self, name, min=0, max='unbounded'):
        self.name = name
        self.min = min
        self.max = max

    def is_bounded(self):
        return self.has_max or self.has_min()

    def has_max(self):
        return (type(self.max) == int)

    def has_min(self):
        return self.min > 0

class ChildContainer:
    def __init__(self, type):
        self.type = type
        self.children = list()

    def add_child(self, child):
        self.children.append(child)

    def select(self, test):
        b = list()
        for c in self.children:
            if test(c):
                b.append(c)

    def size(self):
        return len(self.children)

    def compile_automata(self):
        a = Automata()

        if self.type == 'bag':
            bounded = self.select(lambda x: x.is_bounded())
            unbounded = self.select(lambda x: not x.is_bounded())

            

class Element(ChildContainer):

    def __init__(self, ns, name, content, comment):
        ChildContainer.__init__(self, "bag")
        self.ns = ns
        self.name = name
        self.content = content
        self.comment = comment
        self.arguments = list()

    def add_argument(self, name, type, mandatory = False):
        self.arguments.append(Argument(name, type, mandatory))


def generate_parser(name, title, elements, compiler, target=''):
    t = Template(file="c_expat.tmpl")
    t.name = name
    t.generator = {"version" : "alpha", "name" : "genesx"}
    t.title = title
    t.elements = elements
    t.start = elements['_start']

    filepattern = re.compile("^<<<<<< ([a-zA-Z0-9_\\-\\.]*) >>>>>>$", re.MULTILINE)
    generated = str(t)
    pos = 0
    outstart = -1
    outtarget = None

    while True:
        m = filepattern.search(generated, pos)
        if not m:
            break
        if outtarget:
            out = generated[outstart:m.start()-1]
            f = open(os.path.join(target, outtarget), "w")
            f.write(out)
            f.close()

        outtarget = m.group(1)
        outstart = m.end()
        pos = outstart

    if outtarget:
        out = generated[outstart:]
        f = open(os.path.join(target, outtarget), "w")
        f.write(out)
        f.close()

# name
# content : text | int | float | boolean
# arguments : (name, type, mandatory)
# children : 
#





