using namespace std;

#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include <dirent.h>
#include "include/ElementNames.hh"
#include <iomanip>

// add header file to the original string stream
// use findDouble() when determining if the constructor is a single isotope or not

enum  OutFilter {characters=1, numbers, NA, symbols};

void GetDataStream( string, std::stringstream&);

void FormatData(std::stringstream& streamS, std::stringstream& streamH, string matCompG4STORK, string matCompMCNP);
void GetMaterialList(std::stringstream& stream, std::vector<string> &matNameList, std::vector<double> &matDensityList, std::vector<double> &matTempList, std::vector<vector<string>> &matElemName, std::vector<vector<double>> &matElemWtPerc,
                    std::vector<vector<int>> &isoElemIndex, std::vector<vector<string>> &isoNameList, std::vector<vector<double>> &isoTempVec, std::vector<vector<double>> &isoAmountVec, std::vector<vector<double>> &isoAbunVec);
void ExtractZA(string fileName, int &Z, int &A);
void MatchMaterials(std::vector<double> &matDensityList, std::vector<double> &matTempList, std::vector<vector<string>> &isoNameList, std::vector<vector<double>> &isoTempVec, std::vector<vector<double>> &isoAmountVec,
                    std::vector<string> &matNameList2, std::vector<double> &matDensityList2, std::vector<double> &matTempList2, std::vector<vector<string>> &matElemName2, std::vector<vector<double>> &matElemWtPerc2,
                    std::vector<vector<int>> &isoElemIndex2, std::vector<vector<string>> &isoNameList2, std::vector<vector<double>> &isoTempVec2, std::vector<vector<double>> &isoAmountVec2, std::vector<vector<double>> &isoAbunVec2, std::vector<int> &matMatchIndex2);
bool MovePastWord(std::stringstream& stream, string word);
string ExtractString(std::stringstream &stream, char delim, int outType=7);

string CreateMacroName(string mcnpFileName, string outDirName);
void SetDataStream( string, std::stringstream&);



int main(int argc, char **argv)
{
    string matCompMCNP, matCompG4STORK, outDirName;
    ElementNames elementNames;
    elementNames.SetElementNames();
    std::stringstream streamMCNP, streamG4NDL;
    string macroFileName;

    //checks to make sure that number of arguments (including the program call) is equal to 4 or greater and that it is even
    if(argc>=4&&(floor(argc/2)==ceil(argc/2)))
    {
        outDirName = argv[1];

        //loops through the given geometry source file, header file pairs and creates a macrofile (to be used by the dopplerbroadpara code) for each of them
        for(int i = 2; i<argc; i+=2)
        {
            matCompMCNP = argv[i];
            matCompG4STORK = argv[i+1];

            // copies the data from the source and header file into a stringstream
            GetDataStream(matCompMCNP, streamMCNP);
            GetDataStream(matCompG4STORK, streamG4NDL);

            // Extracts the isotope names and temperatures used in the geometry and stores the information into the source stream
            FormatData(streamG4NDL, streamMCNP, matCompG4STORK, matCompMCNP);

            // generates the name for the macrofile based off the given source file name and the output directory
            macroFileName = CreateMacroName(matCompMCNP, outDirName);

            //stores the information contianed in the source stream into the newly created macrofile
            SetDataStream( macroFileName, streamG4NDL);
        }
    }
    else
    {
        cout << "\nGive the the output directory and then the name of the MCNP MatComp file and the G4STORK MatComp file (in that order) for each G4Stork geometry that you want to compare\n" <<  endl;
    }

    elementNames.ClearStore();
}

void GetDataStream( string mcnpFileName, std::stringstream& ss)
{
    string* data=NULL;

    // Use regular text file
    std::ifstream thefData( mcnpFileName.c_str() , std::ios::in | std::ios::ate );
    if ( thefData.good() )
    {
        // determines the size of the file in characters
        int file_size = thefData.tellg();
        thefData.seekg( 0 , std::ios::beg );

        // creates a character array based off the size of the file
        char* filedata = new char[ file_size ];
        while ( thefData )
        {
            // stores the file data into the character array
            thefData.read( filedata , file_size );
        }
        thefData.close();
        // stores the character array into a string
        data = new string ( filedata , file_size );
        delete [] filedata;
    }
    else
    {
    // found no data file
    //                 set error bit to the stream
        ss.setstate( std::ios::badbit );
    }
    if (data != NULL)
    {
        //stores the string into a stringstream
        ss.str(*data);
        if(data->back()!='\n')
            ss << "\n";
        ss.seekg( 0 , std::ios::beg );
    }

    delete data;
}

void FormatData(std::stringstream& stream, std::stringstream& stream2, string matCompG4STORK, string matCompMCNP)
{
    std::vector<int> matMatchIndex2;
    std::vector<double> matDensityList, matDensityList2, matTempList, matTempList2;
    std::vector<string> matNameList, matNameList2;
    std::vector<vector<int>> isoElemIndex, isoElemIndex2;
    std::vector<vector<string>> matElemName, matElemName2, isoNameList, isoNameList2;
    std::vector<vector<double>> matElemWtPerc, matElemWtPerc2, isoTempVec, isoTempVec2, isoAmountVec, isoAmountVec2, isoAbunVec, isoAbunVec2;

    //Creates a material list with corresponding list of isotope names and properties from the given geometry MatComp file
    GetMaterialList(stream, matNameList, matDensityList, matTempList, matElemName, matElemWtPerc, isoElemIndex, isoNameList, isoTempVec, isoAmountVec, isoAbunVec);
    GetMaterialList(stream2, matNameList2, matDensityList2, matTempList2, matElemName2, matElemWtPerc2, isoElemIndex2, isoNameList2, isoTempVec2, isoAmountVec2, isoAbunVec2);

    //Matches the material data
    MatchMaterials(matDensityList, matTempList, isoNameList, isoTempVec, isoAmountVec, matNameList2, matDensityList2, matTempList2, matElemName2,
                    matElemWtPerc2, isoElemIndex2, isoNameList2, isoTempVec2, isoAmountVec2, isoAbunVec2, matMatchIndex2);

    //Sort the element and isotope data for the matching materials so that they line up and fill in element or isotope data that is present in one material but not the other
    bool match;
    for(int i=0; i<int(matDensityList2.size()); i++)
    {
        if(matMatchIndex2[i]!=-1)
        {
            for(int j=0; j<int(matElemName2[i].size()); j++)
            {
                match=false;
                for(int k=0; k<int(matElemName[matMatchIndex2[i]].size()); k++)
                {
                    if(matElemName2[i][j]==matElemName[matMatchIndex2[i]][k])
                    {
                        if(j!=k)
                        {
                            matElemName[matMatchIndex2[i]].insert(matElemName[matMatchIndex2[i]].begin()+j, matElemName[matMatchIndex2[i]][k]);
                            matElemName[matMatchIndex2[i]].erase(matElemName[matMatchIndex2[i]].begin()+k+1);
                            matElemWtPerc[matMatchIndex2[i]].insert(matElemWtPerc[matMatchIndex2[i]].begin()+j, matElemWtPerc[matMatchIndex2[i]][k]);
                            matElemWtPerc[matMatchIndex2[i]].erase(matElemWtPerc[matMatchIndex2[i]].begin()+k+1);
                            for(int l=0; l<int(isoElemIndex[matMatchIndex2[i]].size()); l++)
                            {
                                if(isoElemIndex[matMatchIndex2[i]][l]==k)
                                {
                                    isoElemIndex[matMatchIndex2[i]][l]=j;
                                }
                            }
                        }
                        match=true;
                        break;
                    }
                }
                if(!match)
                {
                    matElemName[matMatchIndex2[i]].insert(matElemName[matMatchIndex2[i]].begin()+j, matElemName2[i][j]);
                    matElemWtPerc[matMatchIndex2[i]].insert(matElemWtPerc[matMatchIndex2[i]].begin()+j, 0.);
                    for(int l=0; l<int(isoElemIndex[matMatchIndex2[i]].size()); l++)
                    {
                        if(isoElemIndex[matMatchIndex2[i]][l]>=j)
                        {
                            isoElemIndex[matMatchIndex2[i]][l]++;
                        }
                    }
                }
            }

            for(int j=0; j<int(matElemName[matMatchIndex2[i]].size()); j++)
            {
                match=false;
                for(int k=0; k<int(matElemName2[i].size()); k++)
                {
                    if(matElemName2[i][k]==matElemName[matMatchIndex2[i]][j])
                    {
                        match=true;
                        break;
                    }
                }
                if(!match)
                {
                    matElemName2[i].insert(matElemName2[i].begin()+j, matElemName[matMatchIndex2[i]][j]);
                    matElemWtPerc2[i].insert(matElemWtPerc2[i].begin()+j, 0.);
                    for(int l=0; l<int(isoElemIndex2[i].size()); l++)
                    {
                        if(isoElemIndex2[i][l]>=j)
                        {
                            isoElemIndex2[i][l]++;
                        }
                    }
                }
            }

            for(int j=0; j<int(isoNameList2[i].size()); j++)
            {
                match=false;
                for(int k=0; k<int(isoNameList[matMatchIndex2[i]].size()); k++)
                {
                    if(isoNameList2[i][j]==isoNameList[matMatchIndex2[i]][k])
                    {
                        if(j!=k)
                        {
                            isoNameList[matMatchIndex2[i]].insert(isoNameList[matMatchIndex2[i]].begin()+j, isoNameList[matMatchIndex2[i]][k]);
                            isoNameList[matMatchIndex2[i]].erase(isoNameList[matMatchIndex2[i]].begin()+k+1);
                            isoTempVec[matMatchIndex2[i]].insert(isoTempVec[matMatchIndex2[i]].begin()+j, isoTempVec[matMatchIndex2[i]][k]);
                            isoTempVec[matMatchIndex2[i]].erase(isoTempVec[matMatchIndex2[i]].begin()+k+1);
                            isoAmountVec[matMatchIndex2[i]].insert(isoAmountVec[matMatchIndex2[i]].begin()+j, isoAmountVec[matMatchIndex2[i]][k]);
                            isoAmountVec[matMatchIndex2[i]].erase(isoAmountVec[matMatchIndex2[i]].begin()+k+1);
                            isoAbunVec[matMatchIndex2[i]].insert(isoAbunVec[matMatchIndex2[i]].begin()+j, isoAbunVec[matMatchIndex2[i]][k]);
                            isoAbunVec[matMatchIndex2[i]].erase(isoAbunVec[matMatchIndex2[i]].begin()+k+1);
                            isoElemIndex[matMatchIndex2[i]].insert(isoElemIndex[matMatchIndex2[i]].begin()+j, isoElemIndex[matMatchIndex2[i]][k]);
                            isoElemIndex[matMatchIndex2[i]].erase(isoElemIndex[matMatchIndex2[i]].begin()+k+1);
                        }
                        match=true;
                        break;
                    }
                }
                if(!match)
                {
                    isoNameList[matMatchIndex2[i]].insert(isoNameList[matMatchIndex2[i]].begin()+j, isoNameList2[i][j]);
                    isoTempVec[matMatchIndex2[i]].insert(isoTempVec[matMatchIndex2[i]].begin()+j, 0.);
                    isoAmountVec[matMatchIndex2[i]].insert(isoAmountVec[matMatchIndex2[i]].begin()+j, 0.);
                    isoAbunVec[matMatchIndex2[i]].insert(isoAbunVec[matMatchIndex2[i]].begin()+j, 0.);
                    isoElemIndex[matMatchIndex2[i]].insert(isoElemIndex[matMatchIndex2[i]].begin()+j, isoElemIndex2[i][j]);
                }
            }

            for(int j=0; j<int(isoNameList[matMatchIndex2[i]].size()); j++)
            {
                match=false;
                for(int k=0; k<int(isoNameList2[i].size()); k++)
                {
                    if(isoNameList2[i][k]==isoNameList[matMatchIndex2[i]][j])
                    {
                        match=true;
                        break;
                    }
                }
                if(!match)
                {
                    isoNameList2[i].insert(isoNameList2[i].begin()+j, isoNameList[matMatchIndex2[i]][j]);
                    isoTempVec2[i].insert(isoTempVec2[i].begin()+j, 0.);
                    isoAmountVec2[i].insert(isoAmountVec2[i].begin()+j, 0.);
                    isoAbunVec2[i].insert(isoAbunVec2[i].begin()+j, 0.);
                    isoElemIndex2[i].insert(isoElemIndex2[i].begin()+j, isoElemIndex[matMatchIndex2[i]][j]);
                }
            }
        }
    }

    stream.str("");
    stream.clear();

    stream << "This file lists the compares the material data of " << matCompG4STORK << " and " << matCompMCNP << '\n' <<
                "For each material attribute the data from " << matCompG4STORK << " is on the left and the data from " << matCompMCNP << " is in the center and the difference "
                << "between the two is on the right\n" << endl;

    std::vector<bool> unUsedMat(matDensityList.size(), true);
    for(int i=0; i<int(matDensityList2.size()); i++)
    {
        if(matMatchIndex2[i]!=-1)
        {
            stream.fill('-');
            stream.precision(7);
            stream << std::setw(95) << std::left << "Material: "+matNameList[matMatchIndex2[i]] << '\n' << endl;

            stream.fill(' ');
            stream << std::setw(20) << std::left << "Density (g/cm3): " << std::setw(15) << std::left << matDensityList[matMatchIndex2[i]] << std::setw(15) << std::left << matDensityList2[i]
                    << std::setw(15) << std::left << matDensityList2[i]-matDensityList[matMatchIndex2[i]] << '\n' << endl;
            stream << std::setw(20) << std::left << "Temperature (K): " << std::setw(15) << std::left << matTempList[matMatchIndex2[i]] << std::setw(15) << std::left << matTempList2[i]
                    << std::setw(15) << std::left << matTempList2[i]-matTempList[matMatchIndex2[i]] << '\n' << endl;

            stream << std::setw(20) << std::left << "Element Name:" << std::setw(45) << std::left << "Weight (%):" << endl;

            for(int j=0; j<int(matElemName2[i].size()); j++)
            {
                stream << std::setw(20) << std::left << matElemName2[i][j] << std::setw(15) << std::left << matElemWtPerc[matMatchIndex2[i]][j] << std::setw(15) << std::left << matElemWtPerc2[i][j] << std::setw(15) << std::left << matElemWtPerc2[i][j]-matElemWtPerc[matMatchIndex2[i]][j] << endl;
            }
            stream << endl;
            stream << std::setw(20) << std::left << "Isotope Name:" << std::setw(45) << std::left << "Abundance (%):" << std::setw(45) << std::left<< "Temperature (K):" << '\n' << endl;

            for(int j=0; j<int(isoNameList2[i].size()); j++)
            {
                stream << std::setw(20) << std::left << isoNameList2[i][j] << std::setw(15) << std::left << isoAbunVec[matMatchIndex2[i]][j] << std::setw(15) << std::left << isoAbunVec2[i][j] << std::setw(15) << std::left << isoAbunVec2[i][j]-isoAbunVec[matMatchIndex2[i]][j];
                stream << std::setw(15) << std::left << isoTempVec[matMatchIndex2[i]][j] << std::setw(15) << std::left << isoTempVec2[i][j] << std::setw(15) << std::left << isoTempVec2[i][j]-isoTempVec[matMatchIndex2[i]][j] << endl;
            }

            stream << "\n";
            unUsedMat[matMatchIndex2[i]] = false;
        }
        else
        {
            stream << "Error: no match found for material # " << matNameList2[i] << " located in the MCNP geometry,\n either one of the materials defined "
                        << "in the GEANT4 geometry file is significantly off, or the material is missing\n" << "The details of the material are shown below" << endl;

            stream.fill('-');
            stream.precision(7);
            stream << std::setw(95) << std::left << "Material: "+matNameList2[i] << '\n' << endl;

            stream.fill(' ');
            stream << std::setw(20) << std::left << "Density (g/cm3):" << std::setw(15) << std::left << matDensityList2[i] << endl;
            stream << std::setw(20) << std::left << "Temperature (K):" << std::setw(15) << std::left << matTempList2[i] << endl;

            stream << std::setw(20) << std::left << "Element Name:" << std::setw(15) << std::left << "Weight (%):" << endl;

            for(int j=0; j<int(matElemName2[i].size()); j++)
            {
                stream << std::setw(20) << std::left << matElemName2[i][j] << std::setw(15) << std::left << matElemWtPerc2[i][j] << endl;
            }

            stream << std::setw(20) << std::left << "Isotope Name:" << std::setw(15) << std::left << "Abundance (%):" << std::setw(15) << std::left<< "Temperature (K):" << '\n' << endl;

            for(int j=0; j<int(isoNameList2[i].size()); j++)
            {
                stream << std::setw(20) << std::left << isoNameList2[i][j] << std::setw(15) << std::left << isoAbunVec2[i][j];
                stream << std::setw(15) << std::left << isoTempVec2[i][j] << endl;
            }

            stream << "\n";
        }
    }

    for(int i=0; i<int(unUsedMat.size()); i++)
    {
        if(unUsedMat[i])
        {
            stream << "Error: no match found for material " << matNameList[i] << " located in the GEANT4 geometry,\n either this material is unnessary or "
                        << "its composition is significantly off\n" << endl;
        }
    }
}

void GetMaterialList(std::stringstream& stream, std::vector<string> &matNameList, std::vector<double> &matDensityList, std::vector<double> &matTempList, std::vector<vector<string>> &matElemName,
                    std::vector<vector<double>> &matElemWtPerc, std::vector<vector<int>> &isoElemIndex, std::vector<vector<string>> &isoNameList, std::vector<vector<double>> &isoTempVec,
                    std::vector<vector<double>> &isoAmountVec, std::vector<vector<double>> &isoAbunVec)
{
    string line, word, word2, elemName;
    stringstream lineSS, wordSS, numConv;
    int matDegen, numIso, Z, A;
    double dens, matMass, isoMass, isoPer, temp;
    std::vector<int> tempZVec, tempElemIndex;
    std::vector<string> tempElemName, tempNameVec, matDensityNameList;
    std::vector<double> tempElemWtPerc, tempTempVec, tempAmountVec, tempIsoMass;

    while(stream.good())
    {
        matDegen=0;

        MovePastWord(stream, "Material");
        while(stream.get()!=':')
        {
            if(!stream.good())
            {
                return;
            }
        }
        matNameList.push_back(ExtractString(stream, '-', 3));

        //now we use Densit instead of Density for compatability
        MovePastWord(stream, "Densit");
        while(stream.get()!=':')
        {
            if(!stream.good())
            {
                return;
            }
        }

        getline(stream, line);
        lineSS.clear();
        lineSS.str(line);
        line=ExtractString(lineSS, '\t', 3);
        while(line!="")
        {
            matDegen++;
            matDensityNameList.push_back(line);
            line=ExtractString(lineSS, '\t', 3);
        }

        while((stream.peek()<'1')||(stream.peek()>'9'))
        {
            stream.get();
        }

        numConv.clear();
        numConv.str(ExtractString(stream, '\n', 2));
        numConv >> numIso;

        while((stream.peek()<'1')||(stream.peek()>'9'))
        {
            stream.get();
        }

        matMass=0.;
        tempNameVec.clear();
        tempAmountVec.clear();
        tempTempVec.clear();
        for(int i=0; i<numIso; i++)
        {
            stream >> word;
            tempNameVec.push_back(word);
            ExtractZA(word, Z, A);
            tempZVec.push_back(Z);

            stream >> word2;
            wordSS.clear();
            wordSS.str(word2);

            numConv.clear();
            if((word2.length()>7)&&(word2.substr(word2.length()-7)=="weight%"))
                numConv.str(ExtractString(wordSS, 'w', 3));
            else
                numConv.str(ExtractString(wordSS, 'a', 3));
            numConv >> isoPer;

            stream >> word;
            wordSS.clear();
            wordSS.str(word);
            numConv.clear();
            numConv.str(ExtractString(wordSS, '\n', 2));
            numConv >> isoMass;
            tempIsoMass.push_back(isoMass);

            if((word2.length()>7)&&(word2.substr(word2.length()-7)=="weight%"))
            {
                tempAmountVec.push_back(isoPer/isoMass);
                matMass+=isoPer/isoMass;

                stream >> word;
                wordSS.clear();
                wordSS.str(word);
                numConv.clear();
                numConv.str(ExtractString(wordSS, 'k', 3));
                numConv >> temp;
                tempTempVec.push_back(temp);

                if(i==numIso-1)
                {
                    matMass=1/matMass;
                    for(int j=0; j<int(tempAmountVec.size()); j++)
                    {
                        tempAmountVec[j]*=matMass;
                    }
                }
            }
            else
            {
                tempAmountVec.push_back(isoPer);
                matMass+=isoPer*isoMass;

                stream >> word;
                wordSS.clear();
                wordSS.str(word);
                numConv.clear();
                numConv.str(ExtractString(wordSS, 'k', 3));
                numConv >> temp;
                tempTempVec.push_back(temp);
            }
        }

        int maxCount=0, maxIndex=0, count;
        for(int i=0; i<int(tempTempVec.size()); i++)
        {
            count=0;
            for(int j=i+1; j<int(tempTempVec.size()); j++)
            {
                if(abs(tempTempVec[i]-tempTempVec[j])/tempTempVec[i]<0.001)
                {
                    count++;
                }
            }
            if(count>maxCount)
            {
                maxCount=count;
                maxIndex=i;
            }
        }

        matTempList.push_back(tempTempVec[maxIndex]);
        for(int i=0; i<matDegen; i++)
        {
            isoAmountVec.push_back(tempAmountVec);
            isoNameList.push_back(tempNameVec);
            isoTempVec.push_back(tempTempVec);
        }

        for(int i=0; i<matDegen; i++)
        {
            wordSS.clear();
            wordSS.str(matDensityNameList[matDensityNameList.size()-matDegen+i]);
            if(ExtractString(wordSS, '\n', 1)=="atomscm")
            {
                wordSS.clear();
                wordSS.str(matDensityNameList[matDensityNameList.size()-matDegen+i]);
                numConv.clear();
                numConv.str(ExtractString(wordSS, 'a', 3));
                numConv >> dens;
                dens*=matMass*(10/6.022141);
                matDensityList.push_back(dens);
            }
            else
            {
                wordSS.clear();
                wordSS.str(matDensityNameList[matDensityNameList.size()-matDegen+i]);
                numConv.clear();
                numConv.str(ExtractString(wordSS, 'g', 3));
                numConv >> dens;
                matDensityList.push_back(dens);
            }
        }

        std::vector<bool> tempElemFirst(tempZVec.size(), true);
        double abunSum;
        for(int i=0; i<int(tempZVec.size()); i++)
        {
            if(tempElemFirst[i])
            {
                abunSum=tempAmountVec[i];
                tempElemWtPerc.push_back(tempAmountVec[i]*tempIsoMass[i]);
                tempElemName.push_back(tempNameVec[i].substr(tempNameVec[i].find_last_of('_')+1));
                tempElemIndex.push_back(tempElemName.size()-1);
                for(int j=i+1; j<int(tempZVec.size()); j++)
                {
                    if(tempZVec[i]==tempZVec[j])
                    {
                        abunSum+=tempAmountVec[j];
                        tempElemIndex.push_back(tempElemName.size()-1);
                        tempElemWtPerc.back() += tempAmountVec[j]*tempIsoMass[j];
                        tempElemFirst[j] = false;
                    }
                }
                for(int j=i+1; j<int(tempZVec.size()); j++)
                {
                    if(tempZVec[i]==tempZVec[j])
                    {
                        tempAmountVec[j]/=abunSum;
                    }
                }
                tempAmountVec[i]/=abunSum;
                tempElemWtPerc.back() /= matMass;
            }
        }

        for(int i=0; i<matDegen; i++)
        {
            isoAbunVec.push_back(tempAmountVec);
            matElemName.push_back(tempElemName);
            matElemWtPerc.push_back(tempElemWtPerc);
            isoElemIndex.push_back(tempElemIndex);
        }

        tempZVec.clear();
        tempElemName.clear();
        tempElemWtPerc.clear();
        tempElemIndex.clear();
        tempIsoMass.clear();
    }
}

//ExtractZA
//extracts the Z and the A isotope numbers from the file name
void ExtractZA(string fileName, int &Z, int &A)
{
    std::size_t startPos=0;
    stringstream ss;
    ElementNames* elementNames;
    while(startPos!=fileName.length() && (fileName[startPos]<'0' || fileName[startPos]>'9'))
        startPos++;

    if(startPos==fileName.length())
    {
        //cout << "### File Name Does Not Contian a Z or an A Value " << fileName << " is Invalid for Broadening ###" << endl;
        Z=A=-1;
    }
    else
    {
    ////
        std::size_t found1 = fileName.find_first_of('_', startPos);
        if (found1==std::string::npos)
        {
            /*cout << "### File Name Does Not Contian a '_', two are needed, one to seperate the Z and A value, \
            and one to seperate the A and the Element name " << fileName << " is Invalid for Broadening ###" << endl;*/
            Z=A=-1;
        }
        else
        {
            std::size_t found2 = fileName.find_first_of('_', found1+1);
            if (found2==std::string::npos)
            {
                /*cout << "### File Name Does Not Contian a second '_', two are needed, one to seperate the Z and A value, \
                and one to seperate the A and the Element name " << fileName << " is Invalid for Broadening ###" << endl;*/
                Z=A=-1;
            }
            else
            {

                ss.str(fileName.substr(startPos, found1));
                ss >> Z;
                ss.str("");
                ss.clear();
                if(((found2-found1-1) > 2) && (fileName[found2-2] == 'm'))
                    ss.str(fileName.substr(found1+1, found2-found1-3));
                else
                    ss.str(fileName.substr(found1+1, found2-found1-1));
                ss >> A;
                ss.str("");
                ss.clear();
                ss.str(fileName.substr(found2+1));
                if (!(elementNames->CheckName(ss.str(), Z)))
                {
                    //cout << "### " << fileName << " does not include the correct element name at the end ###" << endl;
                    Z=A=-1;
                }
                ss.str("");
                ss.clear();
            }

        }

    }
}

void MatchMaterials(std::vector<double> &matDensityList, std::vector<double> &matTempList, std::vector<vector<string>> &isoNameList, std::vector<vector<double>> &isoTempVec, std::vector<vector<double>> &isoAmountVec,
                    std::vector<string> &matNameList2, std::vector<double> &matDensityList2, std::vector<double> &matTempList2, std::vector<vector<string>> &matElemName2, std::vector<vector<double>> &matElemWtPerc2,
                    std::vector<vector<int>> &isoElemIndex2, std::vector<vector<string>> &isoNameList2, std::vector<vector<double>> &isoTempVec2, std::vector<vector<double>> &isoAmountVec2, std::vector<vector<double>> &isoAbunVec2, std::vector<int> &matMatchIndex2)
{
    int tempMatch;
    vector<int> isoMatch, densMatch;
    double match;
    vector<double> isoMatchNumList;

    matMatchIndex2.assign(matDensityList2.size(),-1.0);

    for(int i=0; i<int(matDensityList.size()); i++)
    {
        isoMatchNumList.assign(1, 0.);
        densMatch.assign(1, 0);
        tempMatch=0;

        for(int j=0; j<int(matDensityList2.size()); j++)
        {
            match=0.;
            for(int k=0; k<int(isoNameList[i].size()); k++)
            {
                for(int l=0; l<int(isoNameList2[j].size()); l++)
                {
                    if(isoNameList[i][k]==isoNameList2[j][l])
                    {
                        match+=1.0-abs(isoAmountVec[i][k]-isoAmountVec2[j][l])/sqrt(isoAmountVec[i][k]);
                        break;
                    }
                }
            }
            match/=max(isoNameList[i].size(),isoNameList2[j].size());
            if((match-isoMatchNumList[0])/isoMatchNumList[0]>0.01)
            {
                isoMatch.clear();
                isoMatch.push_back(j);

                isoMatchNumList.clear();
                isoMatchNumList.push_back(match);
            }
            else if(match>=isoMatchNumList[0])
            {
                isoMatch.push_back(j);
                isoMatchNumList.push_back(match);
            }
        }
        if(isoMatchNumList[0]<0.8)
        {
            continue;
        }
        for(int j=1; j<int(isoMatch.size()); j++)
        {
            if(abs(matDensityList[i]-matDensityList2[isoMatch[j]])<abs(matDensityList[i]-matDensityList2[isoMatch[densMatch[0]]]))
            {
                densMatch.clear();
                densMatch.push_back(j);
            }
            else if(abs(matDensityList[i]-matDensityList2[isoMatch[j]])==abs(matDensityList[i]-matDensityList2[isoMatch[densMatch[0]]]))
            {
                densMatch.push_back(j);
            }
        }
        for(int j=1; j<int(densMatch.size()); j++)
        {
            if(abs(matTempList[i]-matTempList2[isoMatch[densMatch[j]]])<abs(matTempList[i]-matTempList2[isoMatch[densMatch[tempMatch]]]))
            {
                tempMatch=j;
            }
        }
        if((matMatchIndex2[isoMatch[densMatch[tempMatch]]]!=-1)&&(matMatchIndex2[isoMatch[densMatch[tempMatch]]]!=i))
        {
            matNameList2.insert(matNameList2.begin()+isoMatch[densMatch[tempMatch]]+1, matNameList2[isoMatch[densMatch[tempMatch]]]);
            matDensityList2.insert(matDensityList2.begin()+isoMatch[densMatch[tempMatch]]+1, matDensityList2[isoMatch[densMatch[tempMatch]]]);
            matTempList2.insert(matTempList2.begin()+isoMatch[densMatch[tempMatch]]+1, matTempList2[isoMatch[densMatch[tempMatch]]]);
            matElemName2.insert(matElemName2.begin()+isoMatch[densMatch[tempMatch]]+1, matElemName2[isoMatch[densMatch[tempMatch]]]);
            matElemWtPerc2.insert(matElemWtPerc2.begin()+isoMatch[densMatch[tempMatch]]+1, matElemWtPerc2[isoMatch[densMatch[tempMatch]]]);
            isoElemIndex2.insert(isoElemIndex2.begin()+isoMatch[densMatch[tempMatch]]+1, isoElemIndex2[isoMatch[densMatch[tempMatch]]]);
            isoNameList2.insert(isoNameList2.begin()+isoMatch[densMatch[tempMatch]]+1, isoNameList2[isoMatch[densMatch[tempMatch]]]);
            isoTempVec2.insert(isoTempVec2.begin()+isoMatch[densMatch[tempMatch]]+1, isoTempVec2[isoMatch[densMatch[tempMatch]]]);
            isoAmountVec2.insert(isoAmountVec2.begin()+isoMatch[densMatch[tempMatch]]+1, isoAmountVec2[isoMatch[densMatch[tempMatch]]]);
            isoAbunVec2.insert(isoAbunVec2.begin()+isoMatch[densMatch[tempMatch]]+1, isoAbunVec2[isoMatch[densMatch[tempMatch]]]);
            matMatchIndex2.insert(matMatchIndex2.begin()+isoMatch[densMatch[tempMatch]]+1, i);
        }
        else
        {
            matMatchIndex2[isoMatch[densMatch[tempMatch]]]=i;
        }
    }
}

// MovePastWord
// breaks up the given string into words then it searches throught the stream ignoring whitespace looking for match between the words extracted from the string and those in the stream
// a match occurs when the stream has the words in the same order as they are in the string and without any words inbetween them
// when a match is found the file pointer is set to the position just after the match and then the function returns true
bool MovePastWord(std::stringstream& stream, string word)
{
    std::vector<string> wordParts;
    int pos=0, start;
    bool check=true, firstPass=true;

    start = stream.tellg();

    for(int i=0; i<int(word.length()); i++)
    {
        if(word[i]==' ')
        {
            if(check)
            {
                pos=i+1;
            }
            else
            {
                wordParts.push_back(word.substr(pos,i-pos));
                pos=i+1;
                check=true;
            }
        }
        else
        {
            check=false;
            if(i==int(word.length()-1))
            {
                wordParts.push_back(word.substr(pos,i-pos+1));
            }
        }
    }

    if(wordParts.size()==0)
    {
        wordParts.push_back(word);
    }

    string wholeWord, partWord;
    check=false;
    char line[256];

    while(!check)
    {
        if(!stream)
        {
            if(firstPass)
            {
                stream.clear();
                stream.seekg(start, std::ios::beg);
                firstPass=false;
            }
            else
            {
                break;
            }
        }
        if(stream.peek()=='/')
        {
            stream.get();
            if(stream.peek()=='/')
            {
                stream.getline(line,256);
            }
            else if(stream.peek()=='*')
            {
                stream.get();
                while(stream)
                {
                    if(stream.get()=='*')
                    {
                        if(stream.get()=='/')
                        {
                            break;
                        }
                    }
                }
            }
        }
        else if(stream.peek()=='\n')
        {
            stream.getline(line,256);
        }
        else if(stream.peek()=='\t')
        {
            stream.get();
        }
        else if(stream.peek()==' ')
        {
            stream.get();
        }
        else
        {
            for(int i=0; i<int(wordParts.size()); i++)
            {
                stream >> wholeWord;
                if(int(wholeWord.length())>=int((wordParts[i]).length()))
                {
                    if(firstPass)
                    {
                        check=(wholeWord==(wordParts[i]));
                        if(!check)
                        {
                            break;
                        }
                    }
                    else
                    {
                        partWord = wholeWord.substr(0, (wordParts[i]).length());
                        check=(partWord==(wordParts[i]));

                        if(check)
                        {
                            stream.seekg((partWord.length()-wholeWord.length()),std::ios_base::cur);
                        }
                        else if(0==i)
                        {
                            partWord = wholeWord.substr(wholeWord.length()-(wordParts[i]).length(), (wordParts[i]).length());
                            check=(partWord==(wordParts[i]));
                        }

                        if(!check)
                        {
                            break;
                        }
                    }

                }
                else
                {
                    break;
                }
            }
        }

    }

    if(!check)
    {
        stream.clear();
        stream.seekg(start, std::ios::beg);
    }

    return check;
}

// ExtractString
// starting from the current position in the stream the function looks through the stream character by character checking if it meets the given format
// and if so adding it to a string which is returned when the delimeter is found
string ExtractString(std::stringstream &stream, char delim, int outType)
{
    string value="";
    bool charOut=false, numOut=false, symOut=false;
    char letter;
    //bool first=true;

    if(outType==0)
    {

    }
    else if(outType==1)
    {
        charOut=true;
    }
    else if(outType==2)
    {
        numOut=true;
    }
    else if(outType==3)
    {
        charOut=true;
        numOut=true;
    }
    else if(outType==4)
    {
        symOut=true;
    }
    else if(outType==5)
    {
        charOut=true;
        symOut=true;
    }
    else if(outType==6)
    {
        numOut=true;
        symOut=true;
    }
    else
    {
        charOut=true;
        numOut=true;
        symOut=true;
    }

    while(stream&&(stream.peek()!=delim))
    {
        letter = stream.get();
        if(((letter>='A')&&(letter<='Z'))||((letter>='a')&&(letter<='z')))
        {
            if(charOut)
            {
                value+=letter;
                //first=true;
            }
            /*else if(first)
            {
                value+=' ';
                first=false;
            }*/
        }
        else if(((letter>='0')&&(letter<='9'))||(letter=='.')||(letter=='-'))
        {
            if(numOut)
            {
                value+=letter;
                //first=true;
            }
            /*else if(first)
            {
                value+=' ';
                first=false;
            }*/
        }
        else
        {
            if(symOut)
            {
                value+=letter;
                //first=true;
            }
            /*else if(first)
            {
                value+=' ';
                first=false;
            }*/
        }
    }
    return value;
}

//CreateMacroName
//Generates the name for the macro file based off the geometry file name and the output directory
string CreateMacroName(string mcnpFileName, string outDirName)
{
    if((mcnpFileName.substr(mcnpFileName.length()-4,4))==".txt")
    {
        mcnpFileName=mcnpFileName.substr(0,mcnpFileName.length()-4);
    }
    size_t pos = mcnpFileName.find_last_of('/');
    if(pos == std::string::npos)
        pos=0;
    else
        pos++;

    if(mcnpFileName.length()>7)
    {
        string test = mcnpFileName.substr(0, 7);
        if(test=="MatComp")
        {
            pos += 7;
        }
    }

    return (outDirName+"G4MCNPMatComp"+mcnpFileName.substr(pos)+".txt");
}

//SetDataStream
//opens the file with the given name and stores the information contianed by the data stream inside of it
void SetDataStream( string macroFileName, std::stringstream& ss)
{
  std::ofstream out( macroFileName.c_str() , std::ios::out | std::ios::trunc );
  if ( ss.good() )
  {
     ss.seekg( 0 , std::ios::end );
     int file_size = ss.tellg();
     ss.seekg( 0 , std::ios::beg );
     char* filedata = new char[ file_size ];

     while ( ss )
     {
        ss.read( filedata , file_size );
        if(!file_size)
        {
            cout << "\n #### Error the size of the stringstream is invalid ###" << endl;
            break;
        }
     }

     out.write(filedata, file_size);
     if (out.fail())
    {
        cout << endl << "writing the ascii data to the output file " << macroFileName << " failed" << endl
             << " may not have permission to delete an older version of the file" << endl;
    }
     out.close();
     delete [] filedata;
  }
  else
  {
// found no data file
//                 set error bit to the stream
     ss.setstate( std::ios::badbit );

     cout << endl << "### failed to write to ascii file " << macroFileName << " ###" << endl;
  }
   ss.str("");
}
