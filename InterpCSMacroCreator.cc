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

void FormatData(std::stringstream& streamS, std::stringstream& streamH);
void GetMaterialList(std::stringstream& stream, std::vector<double> &matDensityList, std::vector<double> &matTempList, std::vector<vector<string>> &isoNameList, std::vector<vector<double>> &isoTempVec, std::vector<vector<double>> &isoAmountVec);
void MatchMaterials(std::vector<double> &matDensityList, std::vector<double> &matTempList, std::vector<vector<string>> &isoNameList, std::vector<vector<double>> &isoTempVec, std::vector<vector<double>> &isoAmountVec,
                    std::vector<double> &matDensityList2, std::vector<double> &matTempList2, std::vector<vector<string>> &isoNameList2, std::vector<vector<double>> &isoTempVec2, std::vector<vector<double>> &isoAmountVec2,
                    std::vector<double> &matTempNameVec2);
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
            FormatData(streamG4NDL, streamMCNP);

            // generates the name for the macrofile based off the given source file name and the output directory
            macroFileName = CreateMacroName(matCompMCNP, outDirName);

            //stores the information contianed in the source stream into the newly created macrofile
            SetDataStream( macroFileName, streamG4NDL);
        }

        cout << "\nMacro file creation is complete, don't forget to fill in the DoppBroad run parameters at the top of the macrofile before using it\n" << endl;
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

void FormatData(std::stringstream& stream, std::stringstream& stream2)
{
    int numIso=0;

    std::vector<double> matDensityList, matDensityList2, matTempList, matTempList2, matTempNameVec2;
    std::vector<vector<string>> isoNameList, isoNameList2;
    std::vector<vector<double>> isoTempVec, isoTempVec2, isoAmountVec, isoAmountVec2;

    //Creates a material list with corresponding list of isotope names and properties from the given geometry MatComp file
    GetMaterialList(stream, matDensityList, matTempList, isoNameList, isoTempVec, isoAmountVec);
    GetMaterialList(stream2, matDensityList2, matTempList2, isoNameList2, isoTempVec2, isoAmountVec2);

    //Matches the material data
    MatchMaterials(matDensityList, matTempList, isoNameList, isoTempVec, isoAmountVec, matDensityList2, matTempList2, isoNameList2, isoTempVec2, isoAmountVec2, matTempNameVec2);

    stream.str("");
    stream.clear();

    for(int i=0; i<int(isoNameList2.size()); i++)
    {
        numIso+=isoNameList2[i].size();
    }
    // prints a list of variables (that will determine what the doppler broadening program will do with the information) the user must fill in after the macrofile has been created
    stream << "(int: # of parameters)\n" << "(string: CS data input file or directory)\n" << "(string: CS data output file or directory)\n"
            << "(bool: use the file in the input directory with the closest temperature)\n" << "(double: use the file in the input directory with this temperature)\n"
            << "[Optional](string: choose either ascii or compressed for the output file type {Default=ascii})\n" << "[Optional](bool: create log file to show progress and errors {Default=false})\n"
            << "[Optional](bool: regenerate any existing doppler broadened data file with the same name {Default=true})\n" << numIso << "\n\n"
            << "Fill in the above parameters and then delete this line before running.\n" << "The order of the parameters must be mantianed,\n"
            << "to enter an option the user must enter the previous options on the list \nleave the number at the bottom this is your # of isotopes\n\n";

    // loops throught the isotope list and adds the
    stream.fill(' ');
    for(int i=0; i<int(isoNameList2.size()); i++)
    {
        for(int j=0; j<int(isoNameList2[i].size()); j++)
        {
            stream << std::setw(20) << std::left << isoNameList2[i][j] << std::setw(20) << std::left << isoTempVec2[i][j] << std::setw(20) << std::left << matTempNameVec2[i] << '\n';
        }
    }
}

void GetMaterialList(std::stringstream& stream, std::vector<double> &matDensityList, std::vector<double> &matTempList, std::vector<vector<string>> &isoNameList, std::vector<vector<double>> &isoTempVec, std::vector<vector<double>> &isoAmountVec)
{
    string line, word, word2;
    stringstream lineSS, wordSS, numConv;
    int matDegen, numIso;
    double dens, matMass, isoMass, isoPer, temp;
    std::vector<string> tempNameVec, matDensityNameList;
    std::vector<double> tempTempVec, tempAmountVec;

    while(stream.good())
    {
        matDegen=0;

        //not we use Densit instead of Density for compatability
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
    }
}

void MatchMaterials(std::vector<double> &matDensityList, std::vector<double> &matTempList, std::vector<vector<string>> &isoNameList, std::vector<vector<double>> &isoTempVec, std::vector<vector<double>> &isoAmountVec,
                    std::vector<double> &matDensityList2, std::vector<double> &matTempList2, std::vector<vector<string>> &isoNameList2, std::vector<vector<double>> &isoTempVec2, std::vector<vector<double>> &isoAmountVec2,
                    std::vector<double> &matTempNameVec2)
{
    int tempMatch;
    vector<int> isoMatch, densMatch;
    double match;
    vector<double> isoMatchNumList;

    matTempNameVec2.assign(matDensityList2.size(),-1.0);

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
            matDensityList2.push_back(matDensityList[i]);
            matTempList2.push_back(matTempList[i]);
            isoNameList2.push_back(isoNameList[i]);
            isoTempVec2.push_back(isoTempVec[i]);
            isoAmountVec2.push_back(isoAmountVec[i]);
            matTempNameVec2.push_back(matTempList[i]);
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
        if((matTempNameVec2[isoMatch[densMatch[tempMatch]]]!=-1)&&(matTempNameVec2[isoMatch[densMatch[tempMatch]]]!=matTempList[i]))
        {
            matDensityList2.insert(matDensityList2.begin()+isoMatch[densMatch[tempMatch]]+1, matDensityList2[isoMatch[densMatch[tempMatch]]]);
            matTempList2.insert(matTempList2.begin()+isoMatch[densMatch[tempMatch]]+1, matTempList2[isoMatch[densMatch[tempMatch]]]);
            isoNameList2.insert(isoNameList2.begin()+isoMatch[densMatch[tempMatch]]+1, isoNameList2[isoMatch[densMatch[tempMatch]]]);
            isoTempVec2.insert(isoTempVec2.begin()+isoMatch[densMatch[tempMatch]]+1, isoTempVec2[isoMatch[densMatch[tempMatch]]]);
            isoAmountVec2.insert(isoAmountVec2.begin()+isoMatch[densMatch[tempMatch]]+1, isoAmountVec2[isoMatch[densMatch[tempMatch]]]);
            matTempNameVec2.insert(matTempNameVec2.begin()+isoMatch[densMatch[tempMatch]]+1, matTempList[i]);
        }
        else
        {
            matTempNameVec2[isoMatch[densMatch[tempMatch]]]=matTempList[i];
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

    return (outDirName+"InterpCSMacro"+mcnpFileName.substr(pos)+".txt");
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
