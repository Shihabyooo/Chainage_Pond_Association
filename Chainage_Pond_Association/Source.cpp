//This prog was written to associate the chainage reach of a setting out sheet with a pond/depression on which the cut volume of
//the reach will be used to fill the depression, based on a simple distance optimization (pick closest).

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "Types.h"

std::vector<Point> drain;
std::vector<Point> ponds;
std::vector<std::string> assignments;
std::vector<std::vector<double>> associationTable; //SourceX, SourceY, TargetX, TargetY
bool * filled;

const char delimiter = ',';

bool createAssociationTable = true;

bool LoadData(std::string * filePath, std::vector<Point> * dataContainer)
{
	std::ifstream file;
	file.open(*filePath);
	
	if(!file.is_open())
	{
		std::cout << "Failed to open " << filePath->c_str() << std::endl;
		return false;
	}

	std::cout << "Attempting to load " << filePath->c_str() << std::endl;

	char cBuffer = ' ';
	std::string sBuffer = "";

	//skip first line
	while (cBuffer != '\n')
		file.read(&cBuffer, sizeof(cBuffer));

	while (!file.eof())
	{
		size_t pos = file.tellg();
		file.read(&cBuffer, sizeof(cBuffer));
		if (file.eof())
			break;
		file.seekg(pos);
		cBuffer = ' ';

		Point newPoint;

		//Read x;
		while (cBuffer != delimiter)
		{
			file.read(&cBuffer, sizeof(cBuffer));
			
			if (cBuffer == delimiter)
				break;

			sBuffer += cBuffer;
		}
		//std::cout << "x: " << sBuffer.c_str() << std::endl; //test
		newPoint.x = atof(sBuffer.c_str());
		sBuffer = "";
		cBuffer = ' ';
		
		//Read y;
		while (cBuffer != delimiter)
		{
			file.read(&cBuffer, sizeof(cBuffer));

			if (cBuffer == delimiter)
				break;

			sBuffer += cBuffer;
		}
		//std::cout << "y: " << sBuffer.c_str() << std::endl; //test
		newPoint.y = atof(sBuffer.c_str());
		sBuffer = "";
		cBuffer = ' ';

		//Read id;
		while (cBuffer != delimiter)
		{
			file.read(&cBuffer, sizeof(cBuffer));

			if (cBuffer == delimiter)
				break;

			sBuffer += cBuffer;
		}
		//std::cout << "id: " <<  sBuffer.c_str() << std::endl; //test
		newPoint.id = atoi(sBuffer.c_str());
		sBuffer = "";
		cBuffer = ' ';

		//Read volume;
		while (cBuffer != delimiter && cBuffer != '\n')
		{
			file.read(&cBuffer, sizeof(cBuffer));

			if (cBuffer == delimiter)
				break;

			sBuffer += cBuffer;
		}
		//std::cout << "vol: " << sBuffer.c_str() << std::endl; //test
		newPoint.volume = atof(sBuffer.c_str());
		sBuffer = "";
		cBuffer = ' ';

		//add point to vector
		dataContainer->push_back(newPoint);
	}

	file.close();

	std::cout << "Finished loading: " << filePath->c_str() << std::endl;


	return true;
}

bool LoadFiles(std::string * settingOutSheet, std::string * pondsCentroids)
{
	if (!LoadData(settingOutSheet, &drain))
		return false;
	
	if (!LoadData(pondsCentroids, &ponds))
		return false;
}

double Distance(size_t pointOrder, size_t pondOrder)
{
	return sqrt(pow(drain[pointOrder].x - ponds[pondOrder].x, 2.0f) + pow(drain[pointOrder].y - ponds[pondOrder].y, 2.0f));;
}

size_t GetClosestPond(size_t pointOrder)
{
	size_t pondOrder = 0;
	long closestDist;
	for (pondOrder = 0; pondOrder < ponds.size(); pondOrder++)
	{
		if (!filled[pondOrder])
		{
			closestDist = Distance(pointOrder, pondOrder);
			pondOrder = pondOrder;
			break;
		}
	}
	
	for (size_t i = pondOrder + 1; i < ponds.size(); i++)
	{
		if (!filled[i])
		{
			long dist = Distance(pointOrder, pondOrder) < closestDist;
			if (dist < closestDist)
			{
				closestDist = dist;
				pondOrder = i;
			}
		}
	}

	return pondOrder;
}

int WriteResults(std::string * outputPath, std::string *outputExtension)
{
	std::ofstream file;
	std::string resultsFilePath = *outputPath;
	resultsFilePath += *outputExtension;

	file.open(resultsFilePath, std::ios::trunc);
	if (!file.is_open())
	{
		std::cout << "Could not open or create output file " << resultsFilePath.c_str() << std::endl;
		return 1;
	}

	file << "x,y,chainage,volume,pondID" << std::endl;

	for (size_t i = 0; i < drain.size(); i++)
	{
		file << std::fixed << drain[i].x << "," << drain[i].y << "," << drain[i].id << "," << drain[i].volume << "," << assignments[i] << std::endl;
	}

	file.close();


	if (!createAssociationTable)
		return 0;
	//Write association table file.
	std::string assocTablePath = *outputPath;
	assocTablePath += "_AssocTable";
	assocTablePath += *outputExtension;

	file.open(assocTablePath, std::ios::trunc);
	if (!file.is_open())
	{
		std::cout << "Could not open or create output file " << assocTablePath.c_str() << std::endl;
		return 2;
	}
	

	file << "Source_X,Source_Y,Target_X,Target_Y" << std::endl;
	for (size_t i = 0; i < associationTable.size(); i++)
		file << std::fixed << associationTable[i][0] << "," << associationTable[i][1] << "," << associationTable[i][2] << "," << associationTable[i][3] << std::endl;
	
	file.close();
	return 0;
}

void AddToAssociationTable(size_t pointOrder, size_t pondOrder)
{
	std::vector<double> newEntry;
	newEntry.push_back(drain[pointOrder].x);
	newEntry.push_back(drain[pointOrder].y);
	newEntry.push_back(ponds[pondOrder].x);
	newEntry.push_back(ponds[pondOrder].y);

	associationTable.push_back(newEntry);
}

int main(int argc, char ** argv)
{
	std::string settingOutSheetPath = "sos.csv";
	std::string pondsPath = "ponds.csv";
	std::string outputPath = "results";
	std::string outputExtension = ".csv";

	if (!LoadFiles(&settingOutSheetPath, &pondsPath))
		return 1;

	////test
	//std::cout << "drain: " << std::endl;
	//for (int i = 0; i < drain.size(); i++)
	//	std::cout << drain[i].x << "  " << drain[i].y << "  " << drain[i].id << "  " << drain[i].volume << std::endl;
	//std::cout << "\n=================================================================" << std::endl;
	//std::cout << "drain: " << std::endl;
	//for (int i = 0; i < ponds.size(); i++)
	//	std::cout << ponds[i].x << "  " << ponds[i].y << "  " << ponds[i].id << "  " << ponds[i].volume << std::endl;
	////end test

	filled = new bool[ponds.size()];
	for (size_t i = 0; i < ponds.size(); i++)
		filled[i] = false;
	
	assignments.push_back(""); //first point in chainage is not assigned to anything (no excavation).
	
	//Loop over chainage points, starting from drain[i]
	for (size_t i = 1; i < drain.size(); i++)
	{
		double cutVolume = drain[i].volume;
		std::string pondsAssigned = "";
		//std::cout << "Working chainage: " << drain[i].id  << std::endl; //test
		while (true)
		{
			//get closest pond
			size_t closestPondID = GetClosestPond(i);

			if (closestPondID == ponds.size() && filled[closestPondID - 1])
			{
				assignments.push_back("");
				break;
			}

			//std::cout << "---New Iteration. Vol: " << cutVolume << ". ClosestPond: " <<closestPondID << std::endl; //test
			
			if (ponds[closestPondID].volume >= cutVolume)
			{
				if (ponds[closestPondID].volume == cutVolume) //may never happen, but still...
					filled[closestPondID] = true;

				pondsAssigned += std::to_string(ponds[closestPondID].id);
				ponds[closestPondID].volume -= cutVolume;
				if (createAssociationTable)
					AddToAssociationTable(i, closestPondID);
				break;
			}
			else
			{
				cutVolume -= ponds[closestPondID].volume;
				pondsAssigned += std::to_string(ponds[closestPondID].id);
				pondsAssigned += "-";
				filled[closestPondID] = true;
				if (createAssociationTable)
					AddToAssociationTable(i, closestPondID);
			}
		}
		
		assignments.push_back(pondsAssigned);

	}

	WriteResults(&outputPath, &outputExtension);


	delete[] filled;

	std::cout << "Press any key to continue" << std::endl;
	std::cin.ignore();
	std::cin.get();
	return 0;
}