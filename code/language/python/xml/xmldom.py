#!/usr/bin/env python

import xml.dom.minidom, sys

def __usage():
    """show help infomation."""
    print "xmldom.py <XML file name>"


def __scanNode(node, level = 0):
    """Scan a node in XML document."""
    msg = node.__class__.__name__
    if node.nodeType == xml.dom.Node.ELEMENT_NODE:
        msg += ", tag: " + node.tagName
    print " " * level * 4, msg

    if node.hasChildNodes:
        for child in node.childNodes:
            __scanNode(child, level + 1)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        __usage()
        sys.exit(0)

    doc = xml.dom.minidom.parse(sys.argv[1])
    __scanNode(doc)

