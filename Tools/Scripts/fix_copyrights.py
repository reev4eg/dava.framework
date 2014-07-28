#!/usr/bin/env python2.6

import os;
import sys;
import os.path;
import pickle;
import zlib;
import string;
import sys;
import subprocess;
import platform;
import re;

excludeDirs = ["Box2D", "Freetype", "Yaml", "ColladaConverter", "ThirdPartyLibs", "Libs", "yaml-cpp", "PSDTool"]
includePaths = {}

replaceString = "\
/*==================================================================================\n\
    Copyright (c) 2008, binaryzebra\n\
    All rights reserved.\n\
\n\
    Redistribution and use in source and binary forms, with or without\n\
    modification, are permitted provided that the following conditions are met:\n\
\n\
    * Redistributions of source code must retain the above copyright\n\
    notice, this list of conditions and the following disclaimer.\n\
    * Redistributions in binary form must reproduce the above copyright\n\
    notice, this list of conditions and the following disclaimer in the\n\
    documentation and/or other materials provided with the distribution.\n\
    * Neither the name of the binaryzebra nor the\n\
    names of its contributors may be used to endorse or promote products\n\
    derived from this software without specific prior written permission.\n\
\n\
    THIS SOFTWARE IS PROVIDED BY THE binaryzebra AND CONTRIBUTORS \"AS IS\" AND\n\
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED\n\
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE\n\
    DISCLAIMED. IN NO EVENT SHALL binaryzebra BE LIABLE FOR ANY\n\
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES\n\
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;\n\
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND\n\
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\n\
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS\n\
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\
=====================================================================================*/\n\
\n"
	
excludeLogFile = open("excludeLog.log", "w");
includeLogFile = open("includeLog.log", "w");



def visit_directory(arg, dirname, names):
	global excludeDirs, includePaths
	# (path, name) = os.path.split(dirname);
	if (string.find(dirname, "$process") != -1):
		return;
	if (string.find(dirname, ".svn") != -1):
		return;
	if (string.find(dirname, ".git") != -1):
		return;
	relPath = os.path.relpath(dirname)
	for exDir in excludeDirs:	
		if (string.find(relPath, exDir) != -1):
			excludeLogFile.write("exclude: " + relPath + "\n");
			#print relPath;
			return;
	#print "include dir: " + relPath
	includeLogFile.write("include: " + relPath + "\n");

	(dirhead, dirtail) = os.path.split(dirname);
	fullpath = os.path.normpath( dirname + "/");
	for fullname in names:
		pathname = fullpath + "/" + fullname;
		if os.path.isdir(pathname): 
			continue;
		if fullname[0] == '.' or fullname[0] == '$':
			continue;
		includePaths[fullname] = os.path.relpath(pathname);		
		supported_exts = [".cpp", ".h", ".hpp", ".mm"];
	return
	
def process_contents(content):
	pattern = re.compile("^/[*][=]+[^=]*[=]+[*]/", re.DOTALL);

	replacedContent = re.subn(pattern, replaceString, content, count=1);

	newContent = replacedContent[0];
	if(replacedContent[1] == 0):
		newContent = replaceString + "\n" + newContent;
		
	return newContent;
	
def process_file(fullname):
	f = open(fullname)
	contents = "";
	try:
		contents = f.read();
	finally:
	    f.close()
	
	contents = process_contents(contents);
	
	f = open(fullname, "wt")
	try: 
		f.write(contents);
	finally:
		f.close();
	
	return;
	
def process_files(arg, dirname, names):
	global excludeDirs, includePaths
	# (path, name) = os.path.split(dirname);
	if (string.find(dirname, "$process") != -1):
		return;
	if (string.find(dirname, ".svn") != -1):
		return;
	relPath = os.path.relpath(dirname)
	for exDir in excludeDirs:	
		if (string.find(relPath, exDir) != -1):
			excludeLogFile.write("exclude: " + relPath + "\n");
			#print relPath
			return;
	#print "include dir: " + relPath
	includeLogFile.write("include: " + relPath + "\n");

	(dirhead, dirtail) = os.path.split(dirname);
	fullpath = os.path.normpath( dirname + "/");
	for fullname in names:
		pathname = fullpath + "/" + fullname;
		if os.path.isdir(pathname): 
			continue;
		if fullname[0] == '.' or fullname[0] == '$':
			continue;
		
		(name, ext) = os.path.splitext(fullname); 
		supported_exts = [".cpp", ".h", ".hpp"];
		if ext in supported_exts:
			process_file(pathname);
			
	return
	
export_script_dir = os.getcwd();
os.path.walk(export_script_dir, visit_directory, None);
os.path.walk(export_script_dir, process_files, None);

excludeLogFile.close();
includeLogFile.close();

#process_file("Animation/AnimatedObject.cpp")