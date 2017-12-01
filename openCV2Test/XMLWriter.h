#pragma once
#include "ColorPixel.h"
#include <string>
#include <map>
#include <iostream>
#include <fstream>

class XMLWriter {
public:
	XMLWriter(string fileName);
	~XMLWriter();
	bool AddPixel(string pixelName, ColorPixel cp);
	bool RemovePixel(string pixelName);
	map<string, ColorPixel>* getPixels();
private:
	map<string, ColorPixel>* points;
	string File = "";

	static bool exists_test(const string& name) {
		struct stat buffer;
		return (stat(name.c_str(), &buffer) == 0);
	}

	static string getValue(string& line) {
		const int from = line.find(">");
		const int to = line.find("</", from);
		string sub = line.substr(from + 1, to - (from + 1));
		return sub;
	}

	bool WriteToXml();
	bool ReadFromXml();
};

XMLWriter::XMLWriter(const string fileName)
	: File{fileName} {
	points = new map<string, ColorPixel>();
	if (exists_test(File)) {
		ReadFromXml();
		for (auto e : *points) {
			cout << e.first << " " << e.second.x << " " << e.second.y << " " << e.second.red << " " << e.second.green << " " << e
				.second.blue << endl;
		}
	}
	else {
		ofstream out = ofstream(File);
		out.close();
	}
}

XMLWriter::~XMLWriter() {}

bool XMLWriter::AddPixel(const string pixelName, const ColorPixel cp) {
	(*points)[pixelName] = cp;
	WriteToXml();
	return true;
}

map<string, ColorPixel>* XMLWriter::getPixels() {
	return points;
}

bool XMLWriter::WriteToXml() {
	ofstream out(File);
	out << "<points>\n";
	for (auto e : *points) {
		out << "<point>\n";
		out << "\t<name>" << e.first << "</name>\n";
		out << "\t<X>" << e.second.x + 25 << "</X>\n";
		out << "\t<Y>" << e.second.y + 25 << "</Y>\n";
		out << "\t<R>" << e.second.red << "</R>\n";
		out << "\t<G>" << e.second.green << "</G>\n";
		out << "\t<B>" << e.second.blue << "</B>\n";
		out << "</point>\n";
	}
	out << "</points>";
	out.close();
	return true;
}

bool XMLWriter::ReadFromXml() {
	ifstream in = ifstream(File);
	string line;


	while (getline(in, line)) {
		if (line.find("points") != string::npos) {
			string name = "";
			ColorPixel cp;
			while (getline(in, line)) {
				if (line.find("point") != string::npos) {
					getline(in, line);
					if (name != "") {
						(*points)[name] = cp;
					}
				}
				if (line.find("name") != string::npos) {
					//name = "C"+stoi(getValue(line));
					name = getValue(line);
				}
				if (line.find("X") != string::npos) {
					cp.x = stoi(getValue(line));
				}
				if (line.find("Y") != string::npos) {
					cp.y = stoi(getValue(line));
				}
				if (line.find("R") != string::npos) {
					cp.red = stoi(getValue(line));
				}
				if (line.find("G") != string::npos) {
					cp.green = stoi(getValue(line));
				}
				if (line.find("B") != string::npos) {
					cp.blue = stoi(getValue(line));
				}
			}
		}
	}
	return true;
}

bool XMLWriter::RemovePixel(const string pixelName) {
	points->erase(pixelName);
	WriteToXml();
	return true;
}
