#pragma once
#include "ColorPixel.h"
#include <string>
#include <map>
#include <iostream>
#include <fstream>

class XMLWriter {
public:
	XMLWriter(std::string fileName);
	~XMLWriter();
	bool AddPixel(std::string pixelName, ColorPixel cp);
	bool RemovePixel(std::string pixelName);
	std::map<std::string, ColorPixel>* getPixels();
private:
	std::map<std::string, ColorPixel>* points;
	std::string File = "";
	bool exists_test(const std::string& name) {
		struct stat buffer;
		return (stat(name.c_str(), &buffer) == 0);
	}
	std::string getValue(std::string& line) {
		std::string sub;
		int from = 0;
		int to = 0;
		from = line.find(">");
		to = line.find("</", from);
		sub = line.substr(from+1, to);
		return sub;
	}
	bool WriteToXml();
	bool ReadFromXml();
};

XMLWriter::XMLWriter(std::string fileName)
	:File{ fileName } {
	points = new std::map<std::string, ColorPixel>();
	if (exists_test(File)) {
		ReadFromXml();
		for (auto e : *points) {
			std::cout << e.first << " " << e.second.x << " " << e.second.y << " " << e.second.red << " " << e.second.green << " " << e.second.blue << std::endl;
		}
	} else {
		std::ofstream out = std::ofstream(File);
		out.close();
	}
}

XMLWriter::~XMLWriter() {}

bool XMLWriter::AddPixel(std::string pixelName, ColorPixel cp) {
	(*points)[pixelName] = cp;
	WriteToXml();
	return true;
}
std::map<std::string, ColorPixel>* XMLWriter::getPixels() {
	return points;
}
bool XMLWriter::WriteToXml() {
	std::ofstream out(File);
	out << "<points>\n";
	for (auto e : *points) {
		out << "<point>\n";
		out << "\t<name>" << e.first << "</name>\n";
		out << "\t<X>" << e.second.x << "</X>\n";
		out << "\t<Y>" << e.second.y << "</Y>\n";
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
	std::ifstream in = std::ifstream(File);
	std::string line;
	

	while (std::getline(in, line)) {
		if (line.find("points") != std::string::npos) {
			std::string name = "";
			ColorPixel cp;
			while (std::getline(in, line)) {
				if (line.find("point") != std::string::npos) {
					std::getline(in, line);
					if (name != "") {
						(*points)[name] = cp;
					}
				}
				if (line.find("name") != std::string::npos) {
					name = "C"+stoi(getValue(line));
				}
				if (line.find("X") != std::string::npos) {
					cp.x = stoi(getValue(line));
				}
				if (line.find("Y") != std::string::npos) {
					cp.y = stoi(getValue(line));
				}
				if (line.find("R") != std::string::npos) {
					cp.red = stoi(getValue(line));
				}
				if (line.find("G") != std::string::npos) {
					cp.green = stoi(getValue(line));
				}
				if (line.find("B") != std::string::npos) {
					cp.blue = stoi(getValue(line));
				}
			}
		}
	}
	return true;
}

bool XMLWriter::RemovePixel(std::string pixelName) {
	points->erase(pixelName);
	WriteToXml();
	return true;
}
