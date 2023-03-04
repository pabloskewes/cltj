#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>           /* Definition of AT_* constants */
#include <sys/stat.h>
#include <string.h>
#include <iostream>
#include <vector>

// basic file operations
#include <iostream>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

typedef unsigned long long ulong;

using namespace std;


std::vector <ulong> readResults (const char *filename) {

	std::vector <ulong> res;

	struct stat statbuf;	
	if (stat(filename,&statbuf) ==-1) {
		char msg[1000]; sprintf(msg,"STAT error at %s:",filename);
		perror(msg);
		return res;
	}
	
	ulong id, numres, querytime;
	FILE * f = fopen(filename,"r");
	if( !f) {
		char msg[1000]; sprintf(msg,"FOPEN error at %s:",filename);
		perror(msg);
		return res;		
	}
	
	while (3== fscanf(f, "%lu;%lu;%lu\n", &id, &numres, &querytime)) {
		res.push_back( numres );			
	}

	fclose(f);
	return res;
}


int outputNumResults_tofile (std::vector <ulong> v, char *outfilename) {
	ofstream myfile;
	myfile.open (outfilename);
	
	// output results
	for (auto it = v.begin(); it != v.end(); ++it){
		myfile << *it << std::endl;
	}
	
	myfile.close();	
	return 0;
}

char *AddExtensionToFile(const char *name, const char *extension) {
	static char newname[10000];
	sprintf(newname, "%s.%s",name,extension);
	return newname;
}
	

int main(int argc, char* argv[])
{

	char *infile1, *infile2;

    if (argc != 3) {printf("\n use as: %s file1 file2 \n", argv[0]); return 0;}
	infile1= argv[1];
	infile2 = argv[2];
	
	
	std::vector<ulong> res1 = readResults(infile1);
	std::vector<ulong> res2 = readResults(infile2);
	
	if (res1 == res2) 
		std::cout << "Results are identical " << std::endl;
	else {
		std::cout << "Files differ at some point " << std::endl;
		//outputNumResults_tofile(res1, AddExtensionToFile(infile1, "diff") );
		//outputNumResults_tofile(res2, AddExtensionToFile(infile2, "diff") );		
		//std::cout << " see files " <<  AddExtensionToFile(infile1, "diff") << " and " << AddExtensionToFile(infile2, "diff") << std::endl;
		
		{
		  // returns length of vector as unsigned int
		  unsigned int len1 = res1.size();
		  unsigned int len2 = res2.size();
		  unsigned int n = (len1<len2) ? len1 : len2;

		  std::cout << "\t Diferences at: " << std::endl;
		  // run for loop from 0 to vecSize
		  uint iguales=0;
		  for(unsigned int i = 0; i < n; i++) {
			  if (res1[i] != res2[i])
				std::cout << "\t\t line " << right << setw(4) << i+1 << " : " << res1[i] << " != " << res2[i] << std::endl;
			  else iguales++;
		  }
		  cout << endl;		
		  cout << "son iguales: " << iguales << " resultados" << std::endl;
		}
	}

	return 0;
}
